
#pragma once

#include "IApplication.hpp"
#include <memory>
#include <thread>
#include <zmq.hpp>

namespace rtypeEngine {
class AApplication : public IApplication {
public:
  AApplication();
  ~AApplication() override;
  void addModule(const std::string &modulePath, const std::string &pubEndpoint,
                 const std::string &subEndpoint, void* sharedZmqContext = nullptr) override;
  void run() override;
  void loop() override {};
  void init() override {};

  void sendMessage(const std::string &topic,
                   const std::string &message) override;
  std::string getMessage(const std::string &topic) override;
  void subscribe(const std::string &topic, MessageHandler handler) override;
  void unsubscribe(const std::string &topic) override;
  void processMessages() override;

protected:
  void initializeMessageBroker();
  void cleanupMessageBroker();
  zmq::context_t _zmqContext;

private:
  std::unique_ptr<zmq::socket_t> _xpubSocket;
  std::unique_ptr<zmq::socket_t> _xsubSocket;
  std::unique_ptr<zmq::socket_t> _publisher;
  std::unique_ptr<zmq::socket_t> _subscriber;
  std::thread _proxyThread;
  bool _isBrokerActive; // Renamed from _brokerInitialized for clarity

protected:                        // Changed from private to protected
  std::string _pubBrokerEndpoint; // Store the actual ZMQ endpoints
  std::string _subBrokerEndpoint;
  bool _isServerMode; // To indicate if this AApplication instance is running as
                      // a server
  bool _isLocalMode = false;  // True if using inproc:// (no network)

  // New method to configure and initialize the ZMQ broker/connections
  void setupBroker(const std::string &baseEndpoint, bool isServer);

  /**
   * @brief Factory method to generate ZMQ endpoint based on mode
   * @param type "pub" or "sub"
   * @param useLocal Force local mode (inproc://)
   * @return Endpoint string (tcp://... or inproc://...)
   */
  std::string getEndpoint(const std::string& type, bool useLocal = false);
};
} // namespace rtypeEngine
