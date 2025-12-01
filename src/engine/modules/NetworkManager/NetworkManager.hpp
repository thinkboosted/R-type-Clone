#pragma once

#include "../AModule.hpp"
#include "INetworkManager.hpp"

#include <asio.hpp>
#include <atomic>
#include <deque>
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
    explicit NetworkManager(const char* pubEndpoint, const char* subEndpoint);
    ~NetworkManager() override;

    void init() override;
    void loop() override;
    void cleanup() override;

    void bind(uint16_t port) override;
    void connect(const std::string& host, uint16_t port) override;
    void disconnect() override;
    void sendNetworkMessage(const std::string& topic, const std::string& payload) override;

    std::optional<NetworkEnvelope> getFirstMessage() override;
    std::optional<NetworkEnvelope> getLastMessage() override;
    std::vector<NetworkEnvelope> getAllMessages() override;

  private:
    using udp = asio::ip::udp;
    using WorkGuard = asio::executor_work_guard<asio::io_context::executor_type>;

    void startIoContext();
    void stopIoContext();
    void registerSubscriptions();
    void handleCommandString(const std::string& commandLine);
    void handleBindRequest(const std::string& payload);
    void handleConnectRequest(const std::string& payload);
    void handleSendRequest(const std::string& payload);
    void startReceive();
    void handleReceive(const std::error_code& ec, std::size_t bytes_transferred);
    void processIncomingBuffer(const std::vector<char>& buffer);
    void enqueueMessage(const NetworkEnvelope& envelope);
    void queueBusMessage(const std::string& topic, const std::string& payload);
    void publishStatus(const std::string& status);
    void publishError(const std::string& error);
    void disconnectInternal();

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

    std::atomic<bool> _ioThreadRunning;
};

extern "C" rtypeEngine::IModule* createModule(const char* pubEndpoint, const char* subEndpoint);

}  // namespace rtypeEngine
