
#include "AModule.hpp"
#include <zmq.hpp>
#include <stdexcept>
#include <iostream>
#include <chrono>

namespace rtypeEngine {

AModule::~AModule() {
    stop();
}

void AModule::start() {
    _running = true;
    _moduleThread = std::thread([this]() {
        if (!_initialized) {
            init();
            _initialized = true;
        }
        while (_running) {
            processMessages();
            loop();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (_initialized) {
            cleanup();
            _initialized = false;
        }
    });
}

void AModule::stop() {
    if (_initialized) {
        cleanup();
        _initialized = false;
    }
    if (_running) {
        _running = false;
    }
    if (_moduleThread.joinable()) {
        _moduleThread.join();
    }
}

AModule::AModule(const char* pubEndpoint, const char* subEndpoint)
    : IModule(pubEndpoint, subEndpoint),
      _context(1),
      _publisher(std::make_unique<zmq::socket_t>(_context, zmq::socket_type::pub)),
      _subscriber(std::make_unique<zmq::socket_t>(_context, zmq::socket_type::sub)),
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
                    handler(payload);
                }
            }
        }
    }
}

}  // namespace rtypeEngine
