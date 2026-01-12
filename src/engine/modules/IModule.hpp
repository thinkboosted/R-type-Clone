
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <utility>

namespace rtypeEngine {
class IApplication;

class IModule {
  public:
    using MessageHandler = std::function<void(const std::string&)>;
    using TopicSubscription = std::pair<std::string, MessageHandler>;

    explicit IModule(const char* pubEndpoint, const char* subEndpoint)
        : _pubEndpoint(pubEndpoint), _subEndpoint(subEndpoint) {}
    virtual ~IModule() = default;
    virtual void start() = 0;
    virtual void init() = 0;
    virtual void loop() = 0;
    virtual void stop() = 0;
    virtual void cleanup() = 0;
    virtual void release() = 0;

    virtual void sendMessage(const std::string& topic, const std::string& message) = 0;
    virtual std::string getMessage(const std::string& topic) = 0;

    virtual void subscribe(const std::string& topic, MessageHandler handler) = 0;
    virtual void unsubscribe(const std::string& topic) = 0;
    virtual void processMessages() = 0;

    /**
     * @brief Inject shared ZeroMQ context (CRITICAL for inproc://)
     * @param context Pointer to shared zmq::context_t owned by GameEngine
     *
     * MUST be called before init() to prevent modules from creating
     * their own context (which breaks inproc:// communication)
     */
    virtual void setZmqContext(void* context) = 0;

  protected:
    std::string _pubEndpoint;
    std::string _subEndpoint;
    std::vector<TopicSubscription> _subscriptions;

    virtual void setPublisherBufferLength(int length) = 0;
    virtual void setSubscriberBufferLength(int length) = 0;
};

extern "C" rtypeEngine::IModule* createModule(const char* pubEndpoint, const char* subEndpoint);
}  // namespace rtypeEngine
