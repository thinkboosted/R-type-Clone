/**
 * @file AModule.hpp
 * @brief Abstract base class for engine modules
 * 
 * @details Provides ZeroMQ-based pub/sub messaging implementation for modules.
 * Each module runs in its own thread and communicates asynchronously.
 * 
 * @section channels Message Channels
 * This base class provides the messaging infrastructure.
 * Derived classes define their specific subscriptions and publications.
 * 
 * @section threading Threading Model
 * - Each module runs in _moduleThread
 * - _running atomic controls the module loop
 * - processMessages() handles incoming messages
 * 
 * @see IModule for the interface definition
 * @see docs/ARCHITECTURE.md for module system details
 */

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

  protected:
    void setPublisherBufferLength(int length) override;
    void setSubscriberBufferLength(int length) override;

    zmq::context_t _context;
    std::unique_ptr<zmq::socket_t> _publisher;
    std::unique_ptr<zmq::socket_t> _subscriber;

    std::thread _moduleThread;
    std::atomic<bool> _running;
    bool _initialized = false;
};

}  // namespace rtypeEngine
