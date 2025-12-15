#pragma once

#include "../AModule.hpp"
#include "INetworkManager.hpp"

#include <asio.hpp>
#include <atomic>
#include <chrono>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace rtypeEngine {

class NetworkManager : public AModule, public INetworkManager {
public:
  explicit NetworkManager(const char *pubEndpoint, const char *subEndpoint);
  ~NetworkManager() override;

  void init() override;
  void loop() override;
  void cleanup() override;

  void bind(uint16_t port) override;
  void connect(const std::string &host, uint16_t port) override;
  void disconnect() override;
  void sendNetworkMessage(const std::string &topic,
                          const std::string &payload) override;

  // Multi-client support
  void sendToClient(uint32_t clientId, const std::string &topic,
                    const std::string &payload) override;
  void broadcast(const std::string &topic, const std::string &payload) override;
  std::vector<ClientInfo> getConnectedClients() override;

  std::optional<NetworkEnvelope> getFirstMessage() override;
  std::optional<NetworkEnvelope> getLastMessage() override;
  std::vector<NetworkEnvelope> getAllMessages() override;

private:
  using udp = asio::ip::udp;
  using WorkGuard = asio::executor_work_guard<asio::io_context::executor_type>;

  // Client session for multi-client tracking
  struct ClientSession {
    uint32_t id;
    udp::endpoint endpoint;
    std::chrono::steady_clock::time_point lastActivity;
    bool connected = true;
  };

  void startIoContext();
  void stopIoContext();
  void registerSubscriptions();
  void handleCommandString(const std::string &commandLine);
  void handleBindRequest(const std::string &payload);
  void handleConnectRequest(const std::string &payload);
  void handleSendRequest(const std::string &payload);
  void handleSendToRequest(const std::string &payload);
  void handleBroadcastRequest(const std::string &payload);
  void startReceive();
  void handleReceive(const std::error_code &ec, std::size_t bytes_transferred);
  void processIncomingBuffer(const std::vector<char> &buffer,
                             const udp::endpoint &senderEndpoint);
  void enqueueMessage(const NetworkEnvelope &envelope);
  void queueBusMessage(const std::string &topic, const std::string &payload);
  void publishStatus(const std::string &status);
  void publishError(const std::string &error);
  void disconnectInternal();

  // Multi-client helpers
  uint32_t getOrCreateClientId(const udp::endpoint &endpoint);
  void updateClientActivity(uint32_t clientId);
  void checkClientTimeouts();
  void sendHeartbeats();
  void sendToEndpoint(const udp::endpoint &endpoint, const std::string &topic,
                      const std::string &payload);

  asio::io_context _ioContext;
  std::unique_ptr<WorkGuard> _workGuard;
  std::thread _ioThread;
  std::shared_ptr<udp::socket> _socket;
  udp::endpoint _remoteEndpoint;
  std::array<char, 65536> _recvBuffer{};

  std::mutex _queueMutex;
  std::deque<NetworkEnvelope> _messageQueue;

  std::mutex _busMutex;
  std::deque<std::pair<std::string, std::string>> _busMessages;

  // Multi-client tracking
  std::mutex _clientsMutex;
  std::map<uint32_t, ClientSession> _clients;
  std::map<std::string, uint32_t> _endpointToClientId;
  uint32_t _nextClientId = 1;
  bool _isServer = false;

  // Heartbeat/Timeout settings
  std::chrono::steady_clock::time_point _lastHeartbeatTime;
  std::chrono::steady_clock::time_point _lastTimeoutCheckTime;
  static constexpr auto HEARTBEAT_INTERVAL = std::chrono::seconds(1);
  static constexpr auto CLIENT_TIMEOUT = std::chrono::seconds(5);

  std::atomic<bool> _ioThreadRunning;
};

extern "C" rtypeEngine::IModule *createModule(const char *pubEndpoint,
                                              const char *subEndpoint);

} // namespace rtypeEngine
