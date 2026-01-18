/**
 * @file AApplication.hpp
 * @brief Abstract base class for the application host
 * 
 * @details Manages the ZeroMQ message broker that connects all modules.
 * The application loads modules and wires their messaging endpoints.
 * 
 * @section broker Message Broker
 * Uses XPUB/XSUB proxy pattern for module communication:
 * - Modules connect as publishers/subscribers
 * - Proxy forwards messages between all modules
 * 
 * @see IApplication for the interface
 * @see docs/ARCHITECTURE.md for architecture details
 */

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
                 const std::string &subEndpoint) override;
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

private:
  zmq::context_t _zmqContext;
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

  // New method to configure and initialize the ZMQ broker/connections
  void setupBroker(const std::string &baseEndpoint, bool isServer);
};
} // namespace rtypeEngine
