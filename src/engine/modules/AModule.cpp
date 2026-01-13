#include "AModule.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <zmq.hpp>
#include "../core/Logger.hpp"

namespace {
std::string moduleName(const rtypeEngine::AModule* module) {
    return module ? typeid(*module).name() : "AModule";
}
} // namespace

namespace rtypeEngine {

AModule::~AModule() {
    // Stop worker thread before tearing down sockets/context
    stop();
    _publisher.reset();
    _subscriber.reset();
    if (_ownsContext && _zmqContext) {
        delete _zmqContext;
        _zmqContext = nullptr;
    }
}

void AModule::start() {
    _running = true;
    _moduleThread = std::thread([this]() {
        const std::string name = moduleName(this);
        
        std::stringstream ss;
        ss << "Start " << name << " TID:" << std::this_thread::get_id();
        Logger::Debug(ss.str());

        if (!_initialized) {
            Logger::Debug("Init " + name);
            init();
            _initialized = true;
        }
        while (_running) {
            processMessages();
            loop();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (_initialized) {
            Logger::Debug("Cleanup " + name);
            cleanup();
            _initialized = false;
        }
        Logger::Debug("Stop " + name);
    });
}

void AModule::stop() {
    _running = false; // Signal the thread to stop

    // Check if we are trying to join the thread from within itself
    if (_moduleThread.get_id() == std::this_thread::get_id()) {
        if (_moduleThread.joinable()) {
            _moduleThread.detach(); // Detach if we are stopping from within the thread
        }
    } else {
        if (_moduleThread.joinable()) {
            _moduleThread.join(); // Wait for the thread to finish its cleanup
        }
    }
    _initialized = false; // Reset initialized state after thread has cleaned up
}

void AModule::release() {
    // Shared_ptr handles deletion, this function is for any other resource cleanup if needed.
    // As of now, the destructor handles all cleanup.
}

void AModule::setZmqContext(void* context) {
    if (!context) {
        throw std::runtime_error("[AModule] setZmqContext: NULL context pointer");
    }

    // If we own our current context, clean it up
    if (_ownsContext && _zmqContext) {
        // Close existing sockets before deleting context
        _publisher.reset();
        _subscriber.reset();
        delete _zmqContext;
    }

    // Use the injected context
    _zmqContext = static_cast<zmq::context_t*>(context);
    _ownsContext = false;  // We don't own this context

    // Recreate sockets with the new context
    _publisher = std::make_unique<zmq::socket_t>(*_zmqContext, zmq::socket_type::pub);
    _subscriber = std::make_unique<zmq::socket_t>(*_zmqContext, zmq::socket_type::sub);

    // Reconnect sockets if needed
    std::string zmqPubEndpoint = _pubEndpoint;
    if (zmqPubEndpoint.find("tcp://") != 0 && zmqPubEndpoint.find("ipc://") != 0 && zmqPubEndpoint.find("inproc://") != 0) {
        zmqPubEndpoint = "tcp://" + zmqPubEndpoint;
    }

    std::string zmqSubEndpoint = _subEndpoint;
    if (zmqSubEndpoint.find("tcp://") != 0 && zmqSubEndpoint.find("ipc://") != 0 && zmqSubEndpoint.find("inproc://") != 0) {
        zmqSubEndpoint = "tcp://" + zmqSubEndpoint;
    }

    try {
        _publisher->connect(zmqSubEndpoint);
        _subscriber->connect(zmqPubEndpoint);
        Logger::Debug("Successfully injected shared ZMQ context and reconnected sockets");
    } catch (const zmq::error_t& e) {
        Logger::Error(std::string("[AModule] setZmqContext connection error: ") + e.what());
        throw;
    }
}

AModule::AModule(const char* pubEndpoint, const char* subEndpoint)
    : IModule(pubEndpoint, subEndpoint),
    _zmqContext(new zmq::context_t(1)),  // Create own context by default
    _ownsContext(true),                   // We own this context
    _publisher(std::make_unique<zmq::socket_t>(*_zmqContext, zmq::socket_type::pub)),
    _subscriber(std::make_unique<zmq::socket_t>(*_zmqContext, zmq::socket_type::sub)),
      _running(false) {

    std::string zmqPubEndpoint = _pubEndpoint;
    if (zmqPubEndpoint.find("tcp://") != 0 && zmqPubEndpoint.find("ipc://") != 0 && zmqPubEndpoint.find("inproc://") != 0) {
        zmqPubEndpoint = "tcp://" + zmqPubEndpoint;
    }

    std::string zmqSubEndpoint = _subEndpoint;
    if (zmqSubEndpoint.find("tcp://") != 0 && zmqSubEndpoint.find("ipc://") != 0 && zmqSubEndpoint.find("inproc://") != 0) {
        zmqSubEndpoint = "tcp://" + zmqSubEndpoint;
    }

    try {
        _publisher->connect(zmqSubEndpoint);  // Publisher connects to SUB endpoint (XSUB socket)
        _subscriber->connect(zmqPubEndpoint);  // Subscriber connects to PUB endpoint (XPUB socket)
    } catch (const zmq::error_t& e) {
        Logger::Error(std::string("ZeroMQ connection error: ") + e.what());
        throw;
    }
}

void AModule::sendMessage(const std::string& topic, const std::string& message) {
    std::string fullMessage = topic + " " + message;

    zmq::message_t zmqMessage(fullMessage.size());
    memcpy(zmqMessage.data(), fullMessage.c_str(), fullMessage.size());
    auto result = _publisher->send(zmqMessage, zmq::send_flags::none);

    Logger::LogTraffic("->", moduleName(this), topic, message);
}

std::string AModule::getMessage(const std::string& topic) {
    zmq::message_t zmqMessage;
    auto result = _subscriber->recv(zmqMessage, zmq::recv_flags::dontwait);

    if (!result) {
        return "";
    }

    std::string fullMessage(static_cast<char*>(zmqMessage.data()), zmqMessage.size());

    // Check if the message actually matches the expected topic
    if (fullMessage.find(topic) != 0) {
        return "";
    }

    size_t spacePos = fullMessage.find(' ');
    if (spacePos != std::string::npos && spacePos + 1 < fullMessage.size()) {
        return fullMessage.substr(spacePos + 1);
    }

    return fullMessage;
}

void AModule::setPublisherBufferLength(int length) {
    _publisher->set(zmq::sockopt::sndhwm, length);
}

void AModule::setSubscriberBufferLength(int length) {
    _subscriber->set(zmq::sockopt::rcvhwm, length);
}

void AModule::subscribe(const std::string& topic, MessageHandler handler) {
    Logger::Debug(moduleName(this) + " subscribing to topic: '" + topic + "'");
    _subscriptions.push_back({topic, handler});
    _subscriber->set(zmq::sockopt::subscribe, topic);
}

void AModule::unsubscribe(const std::string& topic) {
    _subscriptions.erase(
        std::remove_if(_subscriptions.begin(), _subscriptions.end(),
            [&topic](const TopicSubscription& sub) {
                return sub.first == topic;
            }),
        _subscriptions.end());
    _subscriber->set(zmq::sockopt::unsubscribe, topic);
}

void AModule::processMessages() {
    while (true) {
        zmq::message_t zmqMessage;
        auto result = _subscriber->recv(zmqMessage, zmq::recv_flags::dontwait);

        if (!result) {
            break;
        }

        std::string fullMessage(static_cast<char*>(zmqMessage.data()), zmqMessage.size());

        for (const auto& subscription : _subscriptions) {
            const std::string& topic = subscription.first;
            const MessageHandler& handler = subscription.second;

            if (fullMessage.find(topic) == 0) {
                if (fullMessage.length() == topic.length() || fullMessage[topic.length()] == ' ') {
                    std::string payload = "";
                    if (fullMessage.length() > topic.length() + 1) {
                        payload = fullMessage.substr(topic.length() + 1);
                    }
                    
                    Logger::LogTraffic("<-", moduleName(this), topic, payload);
                    
                    handler(payload);
                }
            }
        }
    }
}

}  // namespace rtypeEngine