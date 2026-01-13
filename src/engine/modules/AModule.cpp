
#include "AModule.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <zmq.hpp>

namespace {
bool debugEnabled() {
    return true;
}

bool sniperDebugEnabled() {
    static bool enabled = (std::getenv("RTYPE_SNIPER_DEBUG") != nullptr);
    return enabled;
}
std::string truncatePayload(const std::string& msg, std::size_t limit = 200) {
    if (msg.size() <= limit) {
        return msg;
    }
    return msg.substr(0, limit) + "...";
}

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
        bool isBullet = (name.find("Bullet") != std::string::npos);
        bool sniper = sniperDebugEnabled() && isBullet;

        if (debugEnabled()) {
            std::cout << "[Module] Start " << name << std::endl;
        }
        if (!_initialized) {
            if (debugEnabled()) {
                std::cout << "[Module] Init " << name << std::endl;
            }
            init();
            _initialized = true;
        }
        while (_running) {
            if (sniper) std::cout << "[Sniper] " << name << " Step 1: Entering processMessages" << std::endl;
            processMessages();
            if (sniper) std::cout << "[Sniper] " << name << " Step 2: Exited processMessages" << std::endl;

            if (sniper) std::cout << "[Sniper] " << name << " Step 3: Entering loop" << std::endl;
            loop();
            if (sniper) std::cout << "[Sniper] " << name << " Step 4: Exited loop" << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (_initialized) {
            if (debugEnabled()) {
                std::cout << "[Module] Cleanup " << name << std::endl;
            }
            cleanup();
            _initialized = false;
        }
        if (debugEnabled()) {
            std::cout << "[Module] Stop " << name << std::endl;
        }
    });
}

void AModule::stop() {
    _running = false; // Signal the thread to stop

    if (_moduleThread.get_id() == std::this_thread::get_id()) {
        if (_moduleThread.joinable()) {
            _moduleThread.detach();
        }
    } else {
        if (_moduleThread.joinable()) {
            _moduleThread.join();
        }
    }
    _initialized = false;
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
        if (debugEnabled()) {
            std::cout << "[AModule] Successfully injected shared ZMQ context and reconnected sockets" << std::endl;
        }
    } catch (const zmq::error_t& e) {
        std::cerr << "[AModule] setZmqContext connection error: " << e.what() << std::endl;
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
        std::cerr << "ZeroMQ connection error: " << e.what() << std::endl;
        throw;
    }
}

void AModule::sendMessage(const std::string& topic, const std::string& message) {
    std::string fullMessage = topic + " " + message;

    zmq::message_t zmqMessage(fullMessage.size());
    memcpy(zmqMessage.data(), fullMessage.c_str(), fullMessage.size());
    auto result = _publisher->send(zmqMessage, zmq::send_flags::none);

    if (debugEnabled()) {
        bool isBinary = (topic == "ImageRendered" || message.size() > 200);
        std::cout << "[Module->] " << moduleName(this) << " " << topic << " | ";
        if (isBinary) {
            std::cout << "[Binary Data / Payload too large: " << message.size() << " bytes]";
        } else {
            std::cout << message;
        }
        std::cout << std::endl;
    }
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
                    if (debugEnabled()) {
                        bool isBinary = (topic == "ImageRendered" || payload.size() > 200);
                        std::cout << "[Module<-] " << moduleName(this) << " " << topic << " | ";
                        if (isBinary) {
                            std::cout << "[Binary Data / Payload too large: " << payload.size() << " bytes]";
                        } else {
                            std::cout << payload;
                        }
                        std::cout << std::endl;
                    }
                    handler(payload);
                }
            }
        }
    }
}

}  // namespace rtypeEngine
