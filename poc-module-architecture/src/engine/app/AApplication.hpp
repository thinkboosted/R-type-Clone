
#pragma once

#include "IApplication.hpp"
#include <zmq.hpp>
#include <memory>
#include <thread>

namespace rtypeEngine {
class AApplication : public IApplication {
  public:
    AApplication();
    ~AApplication() override;
    void addModule(const std::string &modulePath, const std::string &pubEndpoint, const std::string &subEndpoint) override;
    void run() override;
    void loop() override {};
    void init() override {};

    void sendMessage(const std::string& topic, const std::string& message) override;
    std::string getMessage(const std::string& topic) override;
    void subscribe(const std::string& topic, MessageHandler handler) override;
    void unsubscribe(const std::string& topic) override;
    void processMessages() override;

  protected:
    void initializeMessageBroker();
    void cleanupMessageBroker();

  private:
    zmq::context_t _zmqContext;
    std::unique_ptr<zmq::socket_t> _xpubSocket;
    std::unique_ptr<zmq::socket_t> _xsubSocket;
    std::unique_ptr<zmq::socket_t> _publisher;
    std::unique_ptr<zmq::socket_t> _subscriber;
    std::thread _proxyThread;
    bool _brokerInitialized;
};
}  // namespace rtypeEngine
