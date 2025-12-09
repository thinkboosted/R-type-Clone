#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace rtypeEngine {

struct NetworkEnvelope {
    std::string topic;
    std::string payload;
    uint32_t clientId = 0;  // 0 = unknown/server, >0 = specific client
};

struct ClientInfo {
    uint32_t id;
    std::string address;
    uint16_t port;
    std::chrono::steady_clock::time_point lastActivity;
    bool connected = true;
};

class INetworkManager {
  public:
    virtual ~INetworkManager() = default;

    virtual void bind(uint16_t port) = 0;
    virtual void connect(const std::string& host, uint16_t port) = 0;
    virtual void disconnect() = 0;
    virtual void sendNetworkMessage(const std::string& topic, const std::string& payload) = 0;

    // Multi-client support
    virtual void sendToClient(uint32_t clientId, const std::string& topic, const std::string& payload) = 0;
    virtual void broadcast(const std::string& topic, const std::string& payload) = 0;
    virtual std::vector<ClientInfo> getConnectedClients() = 0;

    virtual std::optional<NetworkEnvelope> getFirstMessage() = 0;
    virtual std::optional<NetworkEnvelope> getLastMessage() = 0;
    virtual std::vector<NetworkEnvelope> getAllMessages() = 0;
};

}  // namespace rtypeEngine
