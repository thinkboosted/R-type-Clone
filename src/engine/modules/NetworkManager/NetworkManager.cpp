#include "NetworkManager.hpp"

#include <msgpack.hpp>

#ifdef _WIN32
  #include <winsock2.h>
#else
  #include <arpa/inet.h>
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>

namespace {

std::string trimString(const std::string &value) {
  auto begin = value.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return "";
  }
  auto end = value.find_last_not_of(" \t\r\n");
  return value.substr(begin, end - begin + 1);
}

std::string toLower(std::string value) {
  std::transform(
      value.begin(), value.end(), value.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

std::string endpointToString(const asio::ip::udp::endpoint &ep) {
  return ep.address().to_string() + ":" + std::to_string(ep.port());
}

struct SerializableEnvelope {
  std::string topic;
  std::string payload;

  SerializableEnvelope() = default;
  explicit SerializableEnvelope(const rtypeEngine::NetworkEnvelope &envelope)
      : topic(envelope.topic), payload(envelope.payload) {}

  rtypeEngine::NetworkEnvelope toEnvelope(uint32_t clientId = 0) const {
    return {topic, payload, clientId};
  }

  MSGPACK_DEFINE(topic, payload);
};

} // namespace

namespace rtypeEngine {

NetworkManager::NetworkManager(const char *pubEndpoint, const char *subEndpoint)
    : AModule(pubEndpoint, subEndpoint), _workGuard(nullptr), _socket(nullptr),
      _ioThreadRunning(false), _isServer(false) {
  auto now = std::chrono::steady_clock::now();
  _lastHeartbeatTime = now;
  _lastTimeoutCheckTime = now;
  _lastOverflowLog = now;
}

NetworkManager::~NetworkManager() { cleanup(); }

void NetworkManager::init() {
  startIoContext();
  registerSubscriptions();
  publishStatus("Ready");
}

void NetworkManager::loop() {
  std::deque<std::pair<std::string, std::string>> toPublish;
  {
    std::lock_guard<std::mutex> lock(_busMutex);
    toPublish.swap(_busMessages);
  }

  for (const auto &entry : toPublish) {
    sendMessage(entry.first, entry.second);
  }

  // Heartbeat and timeout checks (only for server)
  if (_isServer && _socket && _socket->is_open()) {
    auto now = std::chrono::steady_clock::now();

    // Send heartbeats
    if (now - _lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
      sendHeartbeats();
      _lastHeartbeatTime = now;
    }

    // Check for client timeouts
    if (now - _lastTimeoutCheckTime >= std::chrono::seconds(1)) {
      checkClientTimeouts();
      _lastTimeoutCheckTime = now;
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void NetworkManager::cleanup() {
  disconnect();
  stopIoContext();

  {
    std::lock_guard<std::mutex> lock(_queueMutex);
    _messageQueue.clear();
  }
  {
    std::lock_guard<std::mutex> lock(_busMutex);
    _busMessages.clear();
  }
  {
    std::lock_guard<std::mutex> lock(_clientsMutex);
    _clients.clear();
    _endpointToClientId.clear();
  }
}

void NetworkManager::startIoContext() {
  _ioContext.restart();
  _workGuard = std::make_unique<WorkGuard>(_ioContext.get_executor());
  _ioThreadRunning = true;

  _ioThread = std::thread([this]() {
    try {
      _ioContext.run();
    } catch (const std::exception &e) {
      std::cerr << "[NetworkManager] IO context error: " << e.what()
                << std::endl;
    }
    _ioThreadRunning = false;
  });
}

void NetworkManager::stopIoContext() {
  if (_workGuard) {
    _workGuard.reset();
  }
  _ioContext.stop();
  if (_ioThread.joinable()) {
    _ioThread.join();
  }
  _ioContext.restart();
}

void NetworkManager::registerSubscriptions() {
  subscribe("NetworkManagerCommand", [this](const std::string &payload) {
    handleCommandString(payload);
  });

  auto bindHandler = [this](const std::string &payload) {
    handleBindRequest(payload);
  };

  subscribe("RequestNetworkBind", bindHandler);
  subscribe("RequestNetworkBinding", bindHandler);

  subscribe("RequestNetworkConnect", [this](const std::string &payload) {
    handleConnectRequest(payload);
  });

  subscribe("RequestNetworkDisconnect",
            [this](const std::string &) { disconnect(); });

  auto sendHandler = [this](const std::string &payload) {
    handleSendRequest(payload);
  };

  subscribe("RequestNetworkMessage", sendHandler);
  subscribe("RequestNetworkSend", sendHandler);

  // Multi-client commands
  subscribe("RequestNetworkSendTo", [this](const std::string &payload) {
    handleSendToRequest(payload);
  });

  subscribe("RequestNetworkBroadcast", [this](const std::string &payload) {
    handleBroadcastRequest(payload);
  });

  // Binary commands
  subscribe("RequestNetworkSendBinary", [this](const std::string &payload) {
    handleSendBinaryRequest(payload);
  });

  subscribe("RequestNetworkBroadcastBinary", [this](const std::string &payload) {
    handleBroadcastBinaryRequest(payload);
  });

  subscribe("RequestNetworkSendToBinary", [this](const std::string &payload) {
    handleSendToBinaryRequest(payload);
  });
}

void NetworkManager::handleCommandString(const std::string &commandLine) {
  std::istringstream iss(commandLine);
  std::string command;
  iss >> command;
  if (command.empty()) {
    return;
  }

  std::string remainder;
  std::getline(iss, remainder);
  remainder = trimString(remainder);

  const std::string lowered = toLower(command);
  if (lowered == "bind") {
    handleBindRequest(remainder);
  } else if (lowered == "connect") {
    handleConnectRequest(remainder);
  } else if (lowered == "disconnect") {
    disconnect();
  } else if (lowered == "send" || lowered == "message") {
    handleSendRequest(remainder);
  } else if (lowered == "sendto") {
    handleSendToRequest(remainder);
  } else if (lowered == "broadcast") {
    handleBroadcastRequest(remainder);
  }
}

void NetworkManager::handleBindRequest(const std::string &payload) {
  const std::string trimmed = trimString(payload);
  if (trimmed.empty()) {
    publishError("BindMissingPort");
    return;
  }

  try {
    int portValue = std::stoi(trimmed);
    if (portValue < 0 || portValue > 65535) {
      throw std::out_of_range("port range");
    }
    bind(static_cast<uint16_t>(portValue));
  } catch (const std::exception &) {
    publishError("BindInvalidPort");
  }
}

void NetworkManager::handleConnectRequest(const std::string &payload) {
  std::istringstream iss(payload);
  std::string host;
  std::string portStr;
  iss >> host >> portStr;

  if (host.empty() || portStr.empty()) {
    publishError("ConnectMissingArguments");
    return;
  }

  try {
    int portValue = std::stoi(portStr);
    if (portValue < 0 || portValue > 65535) {
      throw std::out_of_range("port range");
    }
    connect(host, static_cast<uint16_t>(portValue));
  } catch (const std::exception &) {
    publishError("ConnectInvalidPort");
  }
}

void NetworkManager::handleSendRequest(const std::string &payload) {
  std::string trimmed = trimString(payload);
  if (trimmed.empty()) {
    publishError("SendEmptyPayload");
    return;
  }

  std::string topic = "NetworkMessage";
  std::string message = trimmed;
  const auto spacePos = trimmed.find(' ');
  if (spacePos != std::string::npos) {
    topic = trimString(trimmed.substr(0, spacePos));
    message = trimString(trimmed.substr(spacePos + 1));
  }

  if (topic.empty()) {
    topic = "NetworkMessage";
  }

  sendNetworkMessage(topic, message);
}

void NetworkManager::handleSendToRequest(const std::string &payload) {
  // Format: "clientId topic message"
  std::istringstream iss(payload);
  std::string clientIdStr, topic, message;
  iss >> clientIdStr >> topic;
  std::getline(iss, message);
  message = trimString(message);

  if (clientIdStr.empty() || topic.empty()) {
    publishError("SendToMissingArguments");
    return;
  }

  try {
    uint32_t clientId = static_cast<uint32_t>(std::stoul(clientIdStr));
    sendToClient(clientId, topic, message);
  } catch (const std::exception &) {
    publishError("SendToInvalidClientId");
  }
}

void NetworkManager::handleBroadcastRequest(const std::string &payload) {
  // Format: "topic message"
  std::string trimmed = trimString(payload);
  if (trimmed.empty()) {
    publishError("BroadcastEmptyPayload");
    return;
  }

  std::string topic = "NetworkMessage";
  std::string message = trimmed;
  const auto spacePos = trimmed.find(' ');
  if (spacePos != std::string::npos) {
    topic = trimString(trimmed.substr(0, spacePos));
    message = trimString(trimmed.substr(spacePos + 1));
  }

  broadcast(topic, message);
}

void NetworkManager::handleSendBinaryRequest(const std::string &payload) {
  if (payload.size() < 4) {
    publishError("SendBinaryInvalidFormat");
    return;
  }

  uint32_t topicLen;
  std::memcpy(&topicLen, payload.data(), 4);

  if (topicLen > 1024) {
    publishError("SendBinaryTopicTooLarge");
    return;
  }

  if (payload.size() < 4 + topicLen) {
    publishError("SendBinaryTruncated");
    return;
  }

  std::string topic(payload.data() + 4, topicLen);
  std::vector<char> binPayload(payload.begin() + 4 + topicLen, payload.end());

  sendNetworkMessageBinary(topic, binPayload);
}

void NetworkManager::handleBroadcastBinaryRequest(const std::string &payload) {
  if (payload.size() < 4) {
    publishError("BroadcastBinaryInvalidFormat");
    return;
  }

  uint32_t topicLen;
  std::memcpy(&topicLen, payload.data(), 4);

  if (topicLen > 1024) {
    publishError("BroadcastBinaryTopicTooLarge");
    return;
  }

  if (payload.size() < 4 + topicLen) {
    publishError("BroadcastBinaryTruncated");
    return;
  }

  std::string topic(payload.data() + 4, topicLen);
  std::vector<char> binPayload(payload.begin() + 4 + topicLen, payload.end());

  broadcastBinary(topic, binPayload);
}

void NetworkManager::handleSendToBinaryRequest(const std::string &payload) {
  if (payload.size() < 8) {
    publishError("SendToBinaryInvalidFormat");
    return;
  }

  uint32_t clientId;
  uint32_t topicLen;
  std::memcpy(&clientId, payload.data(), 4);
  std::memcpy(&topicLen, payload.data() + 4, 4);

  if (topicLen > 1024) {
    publishError("SendToBinaryTopicTooLarge");
    return;
  }

  if (payload.size() < 8 + topicLen) {
    publishError("SendToBinaryTruncated");
    return;
  }

  std::string topic(payload.data() + 8, topicLen);
  std::vector<char> binPayload(payload.begin() + 8 + topicLen, payload.end());

  sendToClientBinary(clientId, topic, binPayload);
}

void NetworkManager::bind(uint16_t port) {
  asio::post(_ioContext, [this, port]() {
    disconnectInternal();
    try {
      _socket = std::make_shared<udp::socket>(_ioContext,
                                              udp::endpoint(udp::v4(), port));
      _isServer = true;
      publishStatus("Bound:" + std::to_string(port));
      startReceive();
    } catch (const std::exception &e) {
      publishError(std::string("BindFailed:") + e.what());
    }
  });
}

void NetworkManager::connect(const std::string &host, uint16_t port) {
  asio::post(_ioContext, [this, host, port]() {
    disconnectInternal();
    _isServer = false;
    auto resolver = std::make_shared<udp::resolver>(_ioContext);

    resolver->async_resolve(
        udp::v4(), host, std::to_string(port),
        [this, resolver](const std::error_code &ec,
                         udp::resolver::results_type results) {
          if (ec) {
            publishError(std::string("ResolveFailed:") + ec.message());
            return;
          }

          try {
            udp::endpoint endpoint = *results.begin();

            _socket = std::make_shared<udp::socket>(_ioContext);
            _socket->open(udp::v4());
            _socket->connect(endpoint);
            {
              std::lock_guard<std::mutex> lock(_remoteEndpointMutex);
              _remoteEndpoint = endpoint;
            }

            publishStatus("Connected:" + endpoint.address().to_string() + ":" +
                          std::to_string(endpoint.port()));
            startReceive();
          } catch (const std::exception &e) {
            publishError(std::string("ConnectFailed:") + e.what());
          }
        });
  });
}

void NetworkManager::disconnect() {
  asio::post(_ioContext, [this]() { disconnectInternal(); });
}

void NetworkManager::disconnectInternal() {
  if (_socket && _socket->is_open()) {
    std::error_code ec;
    _socket->close(ec);
  }
  _socket.reset();
  _isServer = false;

  std::lock_guard<std::mutex> lock(_clientsMutex);
  _clients.clear();
  _endpointToClientId.clear();
}

void NetworkManager::startReceive() {
  if (!_socket || !_socket->is_open()) {
    return;
  }

  _socket->async_receive_from(
      asio::buffer(_recvBuffer), _remoteEndpoint,
      [this](const std::error_code &ec, std::size_t bytes_transferred) {
        handleReceive(ec, bytes_transferred);
      });
}

void NetworkManager::handleReceive(const std::error_code &ec,
                                   std::size_t bytes_transferred) {
  if (ec) {
    if (ec != asio::error::operation_aborted) {
      publishError(std::string("ReceiveFailed:") + ec.message());
    }
    return;
  }

  if (bytes_transferred > 0) {
    std::vector<char> data(_recvBuffer.begin(),
                           _recvBuffer.begin() + bytes_transferred);
    udp::endpoint senderEndpoint = _remoteEndpoint;
    processIncomingBuffer(data, senderEndpoint);
  }

  startReceive();
}

void NetworkManager::processIncomingBuffer(
    const std::vector<char> &buffer, const udp::endpoint &senderEndpoint) {
  try {
    msgpack::object_handle handle =
        msgpack::unpack(buffer.data(), buffer.size());
    const msgpack::object &obj = handle.get();
    SerializableEnvelope wireEnvelope;
    obj.convert(wireEnvelope);

    // Track client if we're server
    uint32_t clientId = 0;
    if (_isServer) {
      clientId = getOrCreateClientId(senderEndpoint);
      updateClientActivity(clientId);
    }

    NetworkEnvelope envelope = wireEnvelope.toEnvelope(clientId);

    if (envelope.topic == "_heartbeat_response") {
      return;
    }

    if (envelope.topic == "_heartbeat") {
      sendToEndpoint(senderEndpoint, "_heartbeat_response", "pong");
      return;
    }

    enqueueMessage(envelope);

    if (_isServer && clientId > 0) {
      queueBusMessage(envelope.topic,
                      std::to_string(clientId) + " " + envelope.payload);
    } else {
      queueBusMessage(envelope.topic, envelope.payload);
    }
  } catch (const std::exception &e) {
    publishError(std::string("InvalidPacket:") + e.what());
  }
}

uint32_t NetworkManager::getOrCreateClientId(const udp::endpoint &endpoint) {
  std::string key = endpointToString(endpoint);
  uint32_t clientId = 0;
  bool isNewClient = false;

  {
    std::lock_guard<std::mutex> lock(_clientsMutex);
    auto it = _endpointToClientId.find(key);
    if (it != _endpointToClientId.end()) {
      return it->second;
    }

    // New client
    clientId = _nextClientId++;
    _endpointToClientId[key] = clientId;

    ClientSession session;
    session.id = clientId;
    session.endpoint = endpoint;
    session.lastActivity = std::chrono::steady_clock::now();
    session.connected = true;
    _clients[clientId] = session;
    isNewClient = true;
  }

  // Notify about new client
  if (isNewClient) {
    queueBusMessage("ClientConnected", std::to_string(clientId) + " " + key);
  }

  return clientId;
}

void NetworkManager::updateClientActivity(uint32_t clientId) {
  bool wasReconnected = false;
  {
    std::lock_guard<std::mutex> lock(_clientsMutex);
    auto it = _clients.find(clientId);
    if (it != _clients.end()) {
      it->second.lastActivity = std::chrono::steady_clock::now();
      if (!it->second.connected) {
        it->second.connected = true;
        wasReconnected = true;
      }
    }
  }

  if (wasReconnected) {
    queueBusMessage("ClientReconnected", std::to_string(clientId));
  }
}

void NetworkManager::checkClientTimeouts() {
  std::lock_guard<std::mutex> lock(_clientsMutex);
  auto now = std::chrono::steady_clock::now();

  for (auto &[clientId, session] : _clients) {
    if (session.connected && (now - session.lastActivity) >= CLIENT_TIMEOUT) {
      session.connected = false;
      queueBusMessage("ClientDisconnected",
                      std::to_string(clientId) + " timeout");
    }
  }
}

void NetworkManager::sendHeartbeats() {
  std::vector<udp::endpoint> endpoints;
  {
    std::lock_guard<std::mutex> lock(_clientsMutex);
    for (const auto &[clientId, session] : _clients) {
      if (session.connected) {
        endpoints.push_back(session.endpoint);
      }
    }
  }

  for (const auto &endpoint : endpoints) {
    sendToEndpoint(endpoint, "_heartbeat", "ping");
  }
}

void NetworkManager::sendToEndpoint(const udp::endpoint &endpoint,
                                    const std::string &topic,
                                    const std::string &payload) {
  NetworkEnvelope envelope{topic, payload, 0};
  msgpack::sbuffer buffer;
  SerializableEnvelope wireEnvelope(envelope);
  msgpack::pack(buffer, wireEnvelope);

  auto packet = std::make_shared<std::vector<char>>(buffer.size());
  std::memcpy(packet->data(), buffer.data(), buffer.size());

  asio::post(_ioContext, [this, packet, endpoint]() {
    if (!_socket || !_socket->is_open()) {
      return;
    }

    _socket->async_send_to(
        asio::buffer(*packet), endpoint,
        [this, packet](const std::error_code &ec, std::size_t) {
          if (ec && ec != asio::error::operation_aborted) {
            publishError(std::string("SendFailed:") + ec.message());
          }
        });
  });
}

void NetworkManager::sendToEndpointBinary(const udp::endpoint &endpoint,
                                          const std::string &topic,
                                          const std::vector<char> &payload) {
  // Convert vector<char> to string for the envelope (which uses string storage)
  // This is safe for binary data.
  std::string payloadStr(payload.begin(), payload.end());
  NetworkEnvelope envelope{topic, payloadStr, 0};
  
  msgpack::sbuffer buffer;
  SerializableEnvelope wireEnvelope(envelope);
  msgpack::pack(buffer, wireEnvelope);

  auto packet = std::make_shared<std::vector<char>>(buffer.size());
  std::memcpy(packet->data(), buffer.data(), buffer.size());

  asio::post(_ioContext, [this, packet, endpoint]() {
    if (!_socket || !_socket->is_open()) {
      return;
    }

    _socket->async_send_to(
        asio::buffer(*packet), endpoint,
        [this, packet](const std::error_code &ec, std::size_t) {
          if (ec && ec != asio::error::operation_aborted) {
            publishError(std::string("SendFailed:") + ec.message());
          }
        });
  });
}

void NetworkManager::enqueueMessage(const NetworkEnvelope &envelope) {
  std::lock_guard<std::mutex> lock(_queueMutex);
  _enqueuedTotal.fetch_add(1, std::memory_order_relaxed);
  _messageQueue.push_back(envelope);

  const auto qSize = _messageQueue.size();
  if (qSize > _maxQueueSizeObserved) {
    _maxQueueSizeObserved = qSize;
  }

  constexpr size_t QUEUE_LIMIT = 4096;

  if (_messageQueue.size() > QUEUE_LIMIT) {
    _messageQueue.pop_front();
    const auto overflowCount = _overflowTotal.fetch_add(1, std::memory_order_relaxed) + 1;
    auto now = std::chrono::steady_clock::now();
    if (now - _lastOverflowLog >= std::chrono::seconds(1)) {
      _lastOverflowLog = now;
      std::ostringstream oss;
      oss << "MessageQueueOverflow: dropped oldest. limit=" << QUEUE_LIMIT
          << " queueSize=" << _messageQueue.size()
          << " maxObserved=" << _maxQueueSizeObserved
          << " enqueuedTotal=" << _enqueuedTotal.load(std::memory_order_relaxed)
          << " overflowTotal=" << overflowCount
          << " lastTopic=" << envelope.topic;
      publishError(oss.str());
    }
  }
}

void NetworkManager::queueBusMessage(const std::string &topic,
                                     const std::string &payload) {
  std::lock_guard<std::mutex> lock(_busMutex);
  _busMessages.emplace_back(topic, payload);
}

void NetworkManager::publishStatus(const std::string &status) {
  queueBusMessage("NetworkStatus", status);
}

void NetworkManager::publishError(const std::string &error) {
  queueBusMessage("NetworkError", error);
}

void NetworkManager::sendNetworkMessage(const std::string &topic,
                                        const std::string &payload) {
  if (_isServer) {
    // Server: broadcast to all clients by default
    broadcast(topic, payload);
  } else {
    // Client: send to server
    udp::endpoint target;
    {
      std::lock_guard<std::mutex> lock(_remoteEndpointMutex);
      target = _remoteEndpoint;
    }
    sendToEndpoint(target, topic, payload);
  }
}

void NetworkManager::sendNetworkMessageBinary(const std::string &topic,
                                              const std::vector<char> &payload) {
  if (_isServer) {
    broadcastBinary(topic, payload);
  } else {
    udp::endpoint target;
    {
      std::lock_guard<std::mutex> lock(_remoteEndpointMutex);
      target = _remoteEndpoint;
    }
    sendToEndpointBinary(target, topic, payload);
  }
}

void NetworkManager::sendToClient(uint32_t clientId, const std::string &topic,
                                  const std::string &payload) {
  std::string errorMsg;
  std::optional<udp::endpoint> endpointOpt;

  {
    std::lock_guard<std::mutex> lock(_clientsMutex);
    auto it = _clients.find(clientId);
    if (it == _clients.end()) {
      errorMsg = "SendToClient:UnknownClient:" + std::to_string(clientId);
    } else if (!it->second.connected) {
      errorMsg = "SendToClient:ClientDisconnected:" + std::to_string(clientId);
    } else {
      endpointOpt = it->second.endpoint;
    }
  }

  if (!errorMsg.empty()) {
    publishError(errorMsg);
    return;
  }

  if (endpointOpt.has_value()) {
    sendToEndpoint(endpointOpt.value(), topic, payload);
  }
}

void NetworkManager::sendToClientBinary(uint32_t clientId,
                                        const std::string &topic,
                                        const std::vector<char> &payload) {
  std::string errorMsg;
  std::optional<udp::endpoint> endpointOpt;

  {
    std::lock_guard<std::mutex> lock(_clientsMutex);
    auto it = _clients.find(clientId);
    if (it == _clients.end()) {
      errorMsg = "SendToClient:UnknownClient:" + std::to_string(clientId);
    } else if (!it->second.connected) {
      errorMsg = "SendToClient:ClientDisconnected:" + std::to_string(clientId);
    } else {
      endpointOpt = it->second.endpoint;
    }
  }

  if (!errorMsg.empty()) {
    publishError(errorMsg);
    return;
  }

  if (endpointOpt.has_value()) {
    sendToEndpointBinary(endpointOpt.value(), topic, payload);
  }
}

void NetworkManager::broadcast(const std::string &topic,
                               const std::string &payload) {
  std::vector<udp::endpoint> endpoints;
  {
    std::lock_guard<std::mutex> lock(_clientsMutex);
    if (_clients.empty()) {
      return;
    }
    for (const auto &[clientId, session] : _clients) {
      if (session.connected) {
        endpoints.push_back(session.endpoint);
      }
    }
  }

  for (const auto &endpoint : endpoints) {
    sendToEndpoint(endpoint, topic, payload);
  }
}

void NetworkManager::broadcastBinary(const std::string &topic,
                                     const std::vector<char> &payload) {
  std::vector<udp::endpoint> endpoints;
  {
    std::lock_guard<std::mutex> lock(_clientsMutex);
    if (_clients.empty()) {
      return;
    }
    for (const auto &[clientId, session] : _clients) {
      if (session.connected) {
        endpoints.push_back(session.endpoint);
      }
    }
  }

  for (const auto &endpoint : endpoints) {
    sendToEndpointBinary(endpoint, topic, payload);
  }
}

std::vector<ClientInfo> NetworkManager::getConnectedClients() {
  std::lock_guard<std::mutex> lock(_clientsMutex);
  std::vector<ClientInfo> result;

  for (const auto &[clientId, session] : _clients) {
    ClientInfo info;
    info.id = session.id;
    info.address = session.endpoint.address().to_string();
    info.port = session.endpoint.port();
    info.lastActivity = session.lastActivity;
    info.connected = session.connected;
    result.push_back(info);
  }

  return result;
}

std::optional<NetworkEnvelope> NetworkManager::getFirstMessage() {
  std::lock_guard<std::mutex> lock(_queueMutex);
  if (_messageQueue.empty()) {
    return std::nullopt;
  }
  NetworkEnvelope envelope = _messageQueue.front();
  _messageQueue.pop_front();
  return envelope;
}

std::optional<NetworkEnvelope> NetworkManager::getLastMessage() {
  std::lock_guard<std::mutex> lock(_queueMutex);
  if (_messageQueue.empty()) {
    return std::nullopt;
  }
  NetworkEnvelope envelope = _messageQueue.back();
  _messageQueue.pop_back();
  return envelope;
}

std::vector<NetworkEnvelope> NetworkManager::getAllMessages() {
  std::vector<NetworkEnvelope> messages;
  std::lock_guard<std::mutex> lock(_queueMutex);
  while (!_messageQueue.empty()) {
    messages.push_back(_messageQueue.front());
    _messageQueue.pop_front();
  }
  return messages;
}

} // namespace rtypeEngine

#ifdef _WIN32
    #define NETWORK_MANAGER_EXPORT __declspec(dllexport)
#else
    #define NETWORK_MANAGER_EXPORT
#endif

extern "C" NETWORK_MANAGER_EXPORT rtypeEngine::IModule *createModule(const char *pubEndpoint,
                                              const char *subEndpoint) {
  return new rtypeEngine::NetworkManager(pubEndpoint, subEndpoint);
}
