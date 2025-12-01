#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace rtypeEngine {

struct NetworkEnvelope {
    std::string topic;
    std::string payload;
};

class INetworkManager {
  public:
    virtual ~INetworkManager() = default;

    virtual void bind(uint16_t port) = 0;
    virtual void connect(const std::string& host, uint16_t port) = 0;
    virtual void disconnect() = 0;
    virtual void sendNetworkMessage(const std::string& topic, const std::string& payload) = 0;

    virtual std::optional<NetworkEnvelope> getFirstMessage() = 0;
    virtual std::optional<NetworkEnvelope> getLastMessage() = 0;
    virtual std::vector<NetworkEnvelope> getAllMessages() = 0;
};

}  // namespace rtypeEngine
