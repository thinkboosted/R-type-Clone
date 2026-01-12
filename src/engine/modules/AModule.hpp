
#pragma once

#include "IModule.hpp"
#include <zmq.hpp>
#include <string>
#include <memory>
#include <thread>
#include <atomic>

namespace rtypeEngine {

class AModule : public IModule {
  public:
    explicit AModule(const char* pubEndpoint, const char* subEndpoint);
    virtual ~AModule();

    void start() override;
    void stop() override;
    void release() override;

    void sendMessage(const std::string& topic, const std::string& message) override;
    std::string getMessage(const std::string& topic) override;

    void subscribe(const std::string& topic, MessageHandler handler) override;
    void unsubscribe(const std::string& topic) override;
    void processMessages() override;
  // ZMQ context dependency injection
  void setZmqContext(void* context) override;


  protected:
    void setPublisherBufferLength(int length) override;
    void setSubscriberBufferLength(int length) override;

    zmq::context_t* _zmqContext;  // Pointer to potentially shared context
    bool _ownsContext;  // True if we created the context, false if injected
    std::unique_ptr<zmq::socket_t> _publisher;
    std::unique_ptr<zmq::socket_t> _subscriber;

    std::thread _moduleThread;
    std::atomic<bool> _running;
    bool _initialized = false;
};

}  // namespace rtypeEngine
