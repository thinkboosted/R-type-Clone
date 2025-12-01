#include "NetworkManager.hpp"

#include <msgpack.hpp>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <arpa/inet.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstring>
#include <iostream>
#include <sstream>

namespace {

std::string trimString(const std::string& value) {
    auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

struct SerializableEnvelope {
    std::string topic;
    std::string payload;

    SerializableEnvelope() = default;
    explicit SerializableEnvelope(const rtypeEngine::NetworkEnvelope& envelope)
        : topic(envelope.topic), payload(envelope.payload) {}

    rtypeEngine::NetworkEnvelope toEnvelope() const {
        return {topic, payload};
    }

    MSGPACK_DEFINE(topic, payload);
};

}  // namespace

namespace rtypeEngine {

NetworkManager::NetworkManager(const char* pubEndpoint, const char* subEndpoint)
    : AModule(pubEndpoint, subEndpoint),
      _workGuard(nullptr),
      _socket(nullptr),
      _ioThreadRunning(false) {}

NetworkManager::~NetworkManager() {
    cleanup();
}

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

    for (const auto& entry : toPublish) {
        sendMessage(entry.first, entry.second);
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
}

void NetworkManager::startIoContext() {
    _ioContext.restart();
    _workGuard = std::make_unique<WorkGuard>(_ioContext.get_executor());
    _ioThreadRunning = true;

    _ioThread = std::thread([this]() {
        try {
            _ioContext.run();
        } catch (const std::exception& e) {
            std::cerr << "[NetworkManager] IO context error: " << e.what() << std::endl;
        }
        _ioThreadRunning = false;
    });
}

void NetworkManager::stopIoContext() {
    if (!_workGuard && !_ioThreadRunning && !_ioThread.joinable()) {
        return;
    }
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
    subscribe("NetworkManagerCommand", [this](const std::string& payload) {
        handleCommandString(payload);
    });

    auto bindHandler = [this](const std::string& payload) {
        handleBindRequest(payload);
    };

    subscribe("RequestNetworkBind", bindHandler);
    subscribe("RequestNetworkBinding", bindHandler);

    subscribe("RequestNetworkConnect", [this](const std::string& payload) {
        handleConnectRequest(payload);
    });

    subscribe("RequestNetworkDisconnect", [this](const std::string&)
 {
        disconnect();
    });

    auto sendHandler = [this](const std::string& payload) {
        handleSendRequest(payload);
    };

    subscribe("RequestNetworkMessage", sendHandler);
    subscribe("RequestNetworkSend", sendHandler);
}

void NetworkManager::handleCommandString(const std::string& commandLine) {
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
    }
}

void NetworkManager::handleBindRequest(const std::string& payload) {
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
    } catch (const std::exception&) {
        publishError("BindInvalidPort");
    }
}

void NetworkManager::handleConnectRequest(const std::string& payload) {
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
    } catch (const std::exception&) {
        publishError("ConnectInvalidPort");
    }
}

void NetworkManager::handleSendRequest(const std::string& payload) {
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

void NetworkManager::bind(uint16_t port) {
    asio::post(_ioContext, [this, port]() {
        disconnectInternal();
        try {
            _socket = std::make_shared<udp::socket>(_ioContext, udp::endpoint(udp::v4(), port));
            publishStatus("Bound:" + std::to_string(port));
            startReceive();
        } catch (const std::exception& e) {
            publishError(std::string("BindFailed:") + e.what());
        }
    });
}

void NetworkManager::connect(const std::string& host, uint16_t port) {
    asio::post(_ioContext, [this, host, port]() {
        disconnectInternal();
        auto resolver = std::make_shared<udp::resolver>(_ioContext);

        resolver->async_resolve(udp::v4(), host, std::to_string(port), [this, resolver](const std::error_code& ec, udp::resolver::results_type results) {
            if (ec) {
                publishError(std::string("ResolveFailed:") + ec.message());
                return;
            }

            try {
                // Take the first endpoint
                udp::endpoint endpoint = *results.begin();

                _socket = std::make_shared<udp::socket>(_ioContext);
                _socket->open(udp::v4());
                // For client, connecting sets the default remote endpoint
                _socket->connect(endpoint);
                _remoteEndpoint = endpoint;

                publishStatus("Connected:" + endpoint.address().to_string() + ":" + std::to_string(endpoint.port()));
                startReceive();
            } catch (const std::exception& e) {
                publishError(std::string("ConnectFailed:") + e.what());
            }
        });
    });
}

void NetworkManager::disconnect() {
    asio::post(_ioContext, [this]() {
        disconnectInternal();
    });
}

void NetworkManager::disconnectInternal() {
    if (_socket && _socket->is_open()) {
        std::error_code ec;
        _socket->close(ec);
    }
    _socket.reset();
}

void NetworkManager::startReceive() {
    if (!_socket || !_socket->is_open()) {
        return;
    }

    _socket->async_receive_from(
        asio::buffer(_recvBuffer), _remoteEndpoint,
        [this](const std::error_code& ec, std::size_t bytes_transferred) {
            handleReceive(ec, bytes_transferred);
        });
}

void NetworkManager::handleReceive(const std::error_code& ec, std::size_t bytes_transferred) {
    if (ec) {
        if (ec != asio::error::operation_aborted) {
            publishError(std::string("ReceiveFailed:") + ec.message());
        }
        return;
    }

    if (bytes_transferred > 0) {
        std::vector<char> data(_recvBuffer.begin(), _recvBuffer.begin() + bytes_transferred);
        processIncomingBuffer(data);
    }

    startReceive();
}

void NetworkManager::processIncomingBuffer(const std::vector<char>& buffer) {
    try {
        msgpack::object_handle handle = msgpack::unpack(buffer.data(), buffer.size());
        const msgpack::object& obj = handle.get();
        SerializableEnvelope wireEnvelope;
        obj.convert(wireEnvelope);
        NetworkEnvelope envelope = wireEnvelope.toEnvelope();

        enqueueMessage(envelope);
        queueBusMessage(envelope.topic, envelope.payload);
    } catch (const std::exception& e) {
        publishError(std::string("InvalidPacket:") + e.what());
    }
}

void NetworkManager::enqueueMessage(const NetworkEnvelope& envelope) {
    std::lock_guard<std::mutex> lock(_queueMutex);
    _messageQueue.push_back(envelope);
    if (_messageQueue.size() > 1024) {
        _messageQueue.pop_front();
    }
}

void NetworkManager::queueBusMessage(const std::string& topic, const std::string& payload) {
    std::lock_guard<std::mutex> lock(_busMutex);
    _busMessages.emplace_back(topic, payload);
}

void NetworkManager::publishStatus(const std::string& status) {
    queueBusMessage("NetworkStatus", status);
}

void NetworkManager::publishError(const std::string& error) {
    queueBusMessage("NetworkError", error);
}

void NetworkManager::sendNetworkMessage(const std::string& topic, const std::string& payload) {
    NetworkEnvelope envelope{topic.empty() ? std::string("NetworkMessage") : topic, payload};
    msgpack::sbuffer buffer;
    SerializableEnvelope wireEnvelope(envelope);
    msgpack::pack(buffer, wireEnvelope);

    auto packet = std::make_shared<std::vector<char>>(buffer.size());
    std::memcpy(packet->data(), buffer.data(), buffer.size());

    asio::post(_ioContext, [this, packet]() {
        if (!_socket || !_socket->is_open()) {
            publishError("SendFailed:SocketUnavailable");
            return;
        }

        _socket->async_send_to(asio::buffer(*packet), _remoteEndpoint, [this, packet](const std::error_code& ec, std::size_t) {
            if (ec) {
                publishError(std::string("SendFailed:") + ec.message());
            }
        });
    });
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

extern "C" rtypeEngine::IModule* createModule(const char* pubEndpoint, const char* subEndpoint) {
    return new NetworkManager(pubEndpoint, subEndpoint);
}

}  // namespace rtypeEngine