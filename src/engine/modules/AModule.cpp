
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
    static bool enabled = (std::getenv("RTYPE_DEBUG") != nullptr);
    return enabled;
}

bool sniperDebugEnabled() {
    static bool enabled = (std::getenv("RTYPE_SNIPER_DEBUG") != nullptr);
    return enabled;
}

bool profileEnabled() {
    static bool enabled = (std::getenv("RTYPE_PROFILE") != nullptr);
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
    stop();
}

void AModule::start() {
    _running = true;
    _moduleThread = std::thread([this]() {
        const std::string name = moduleName(this);
        bool isBullet = (name.find("Bullet") != std::string::npos);
        bool sniper = sniperDebugEnabled() && isBullet;
        auto lastProfileLog = std::chrono::steady_clock::now();
        double processMsTotal = 0.0;
        double loopMsTotal = 0.0;
        int profileSamples = 0;

        if (debugEnabled()) {
            std::cout << "[Module] Start " << name << std::endl;
        }
        if (!_initialized) {
            if (debugEnabled()) {
                std::cout << "[Module] Init " << name << std::endl;
            }
            try {
                init();
                _initialized = true;
            } catch (const std::exception& e) {
                std::cerr << "[Module] Init failed for " << name << ": " << e.what() << std::endl;
                _running = false;
            } catch (...) {
                std::cerr << "[Module] Init failed for " << name << ": unknown error" << std::endl;
                _running = false;
            }
        }
        while (_running) {
            if (sniper) std::cout << "[Sniper] " << name << " Step 1: Entering processMessages" << std::endl;
            const auto processStart = std::chrono::steady_clock::now();
            try {
                processMessages();
            } catch (const std::exception& e) {
                std::cerr << "[Module] Message processing error in " << name << ": " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "[Module] Message processing error in " << name << ": unknown error" << std::endl;
            }
            const auto processEnd = std::chrono::steady_clock::now();
            if (sniper) std::cout << "[Sniper] " << name << " Step 2: Exited processMessages" << std::endl;

            if (sniper) std::cout << "[Sniper] " << name << " Step 3: Entering loop" << std::endl;
            const auto loopStart = std::chrono::steady_clock::now();
            try {
                loop();
            } catch (const std::exception& e) {
                std::cerr << "[Module] Loop error in " << name << ": " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "[Module] Loop error in " << name << ": unknown error" << std::endl;
            }
            const auto loopEnd = std::chrono::steady_clock::now();
            if (sniper) std::cout << "[Sniper] " << name << " Step 4: Exited loop" << std::endl;

            if (profileEnabled()) {
                processMsTotal += std::chrono::duration<double, std::milli>(processEnd - processStart).count();
                loopMsTotal += std::chrono::duration<double, std::milli>(loopEnd - loopStart).count();
                ++profileSamples;
                if (loopEnd - lastProfileLog >= std::chrono::seconds(1)) {
                    const double sampleCount = profileSamples > 0 ? static_cast<double>(profileSamples) : 1.0;
                    std::cout << "[PROFILE][Module] " << name
                              << " process_avg_ms=" << (processMsTotal / sampleCount)
                              << " loop_avg_ms=" << (loopMsTotal / sampleCount)
                              << " samples=" << profileSamples << std::endl;
                    processMsTotal = 0.0;
                    loopMsTotal = 0.0;
                    profileSamples = 0;
                    lastProfileLog = loopEnd;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        if (_initialized) {
            if (debugEnabled()) {
                std::cout << "[Module] Cleanup " << name << std::endl;
            }
            try {
                cleanup();
            } catch (const std::exception& e) {
                std::cerr << "[Module] Cleanup error in " << name << ": " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "[Module] Cleanup error in " << name << ": unknown error" << std::endl;
            }
            _initialized = false;
        }
        if (debugEnabled()) {
            std::cout << "[Module] Stop " << name << std::endl;
        }
    });
}

void AModule::stop() {
    _running = false; // Signal the thread to stop

    if (_moduleThread.joinable()) {
        _moduleThread.join(); // Wait for the thread to finish its cleanup
    }
    _initialized = false; // Reset initialized state after thread has cleaned up
}

void AModule::release() {
    // Shared_ptr handles deletion, this function is for any other resource cleanup if needed.
    // As of now, the destructor handles all cleanup.
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
    }
}

void AModule::sendMessage(const std::string& topic, const std::string& message) {
    std::string fullMessage = topic + " " + message;

    zmq::message_t zmqMessage(fullMessage.size());
    memcpy(zmqMessage.data(), fullMessage.c_str(), fullMessage.size());
    auto result = _publisher->send(zmqMessage, zmq::send_flags::none);

    if (debugEnabled()) {
        std::cout << "[Module->] " << moduleName(this) << " " << topic << " | " << truncatePayload(message) << std::endl;
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
    int messagesProcessed = 0;
    constexpr int maxMessagesPerLoop = 256;
    while (messagesProcessed < maxMessagesPerLoop) {
        zmq::message_t zmqMessage;
        auto result = _subscriber->recv(zmqMessage, zmq::recv_flags::dontwait);

        if (!result) {
            break;
        }
        ++messagesProcessed;

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
                        std::cout << "[Module<-] " << moduleName(this) << " " << topic << " | " << truncatePayload(payload) << std::endl;
                    }
                    try {
                        handler(payload);
                    } catch (const std::exception& e) {
                        std::cerr << "[Module] Handler error for topic '" << topic << "': " << e.what() << std::endl;
                    } catch (...) {
                        std::cerr << "[Module] Handler error for topic '" << topic << "': unknown error" << std::endl;
                    }
                }
            }
        }
    }
}

}  // namespace rtypeEngine
