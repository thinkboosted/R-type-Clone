
#include "AApplication.hpp"
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <stdexcept> // For std::stoi
#include <string>

namespace {
bool debugEnabled() {
  static bool enabled = (std::getenv("RTYPE_DEBUG") != nullptr);
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

bool startsWith(const std::string &value, const std::string &prefix) {
  return value.rfind(prefix, 0) == 0;
}

void cleanupIpcSocketPath(const std::string &endpoint) {
  if (!startsWith(endpoint, "ipc://")) {
    return;
  }
  const std::string path = endpoint.substr(6);
  if (!path.empty()) {
    std::remove(path.c_str());
  }
}
} // namespace

namespace rtypeEngine {

AApplication::AApplication()
    : _zmqContext(1),
      _xpubSocket(nullptr),
      _xsubSocket(nullptr),
      _publisher(nullptr),
      _subscriber(nullptr),
      _isBrokerActive(false),
      _isServerMode(false) {}

AApplication::~AApplication() {
  cleanupMessageBroker();
}

void AApplication::setupBroker(const std::string& baseEndpoint, bool isServer) {
    _isServerMode = isServer;

  // IPC mode: use a namespace and derive dedicated pub/sub socket paths.
  // Example: ipc:///tmp/rtype-client-bus-1234 ->
  //          ipc:///tmp/rtype-client-bus-1234-pub / ...-sub
  if (baseEndpoint.find("ipc://") == 0) {
    _pubBrokerEndpoint = baseEndpoint + "-pub";
    _subBrokerEndpoint = baseEndpoint + "-sub";
  } else if (baseEndpoint.find(":*") != std::string::npos) {
    // Wildcard mode (likely client with ephemeral ports)
    // We can't calculate port+1, so we just use wildcard for both.
    // ZeroMQ will assign two different random ports.
    _pubBrokerEndpoint = baseEndpoint;
    _subBrokerEndpoint = baseEndpoint; // Will bind to a new random port
  } else {
    size_t colonPos = baseEndpoint.find_last_of(':');
    if (colonPos != std::string::npos) {
      std::string base = baseEndpoint.substr(0, colonPos);
      int port = 0;
      try {
        port = std::stoi(baseEndpoint.substr(colonPos + 1));
      } catch (const std::exception &e) {
        std::cerr << "Invalid port in baseEndpoint, using same endpoint for pub/sub: " << e.what() << std::endl;
        _pubBrokerEndpoint = baseEndpoint;
        _subBrokerEndpoint = baseEndpoint;
        port = -1;
      }
      if (port >= 0) {
        _pubBrokerEndpoint = base + ":" + std::to_string(port);
        _subBrokerEndpoint = base + ":" + std::to_string(port + 1);
      }
    } else {
      _pubBrokerEndpoint = baseEndpoint;
      _subBrokerEndpoint = baseEndpoint;
    }
  }


    if (_pubBrokerEndpoint.find("tcp://") != 0 && _pubBrokerEndpoint.find("ipc://") != 0 && _pubBrokerEndpoint.find("inproc://") != 0) {
        _pubBrokerEndpoint = "tcp://" + _pubBrokerEndpoint;
    }
    if (_subBrokerEndpoint.find("tcp://") != 0 && _subBrokerEndpoint.find("ipc://") != 0 && _subBrokerEndpoint.find("inproc://") != 0) {
        _subBrokerEndpoint = "tcp://" + _subBrokerEndpoint;
    }

    if (_isServerMode) {
        try {
            _xpubSocket = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::xpub);
            _xsubSocket = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::xsub);
            _publisher = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::pub);
            _subscriber = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::sub);

        // Prevent stale IPC socket files from previous crashes from blocking bind().
        cleanupIpcSocketPath(_pubBrokerEndpoint);
        cleanupIpcSocketPath(_subBrokerEndpoint);

            _xpubSocket->bind(_pubBrokerEndpoint);
            // If we bound to a wildcard port, update the endpoint with the actual assigned port
            if (_pubBrokerEndpoint.find(":*") != std::string::npos) {
                _pubBrokerEndpoint = _xpubSocket->get(zmq::sockopt::last_endpoint);
            }

            _xsubSocket->bind(_subBrokerEndpoint);
            if (_subBrokerEndpoint.find(":*") != std::string::npos) {
                _subBrokerEndpoint = _xsubSocket->get(zmq::sockopt::last_endpoint);
            }

            // AApplication's own publisher/subscriber connect to its internal broker
            _publisher->connect(_subBrokerEndpoint);
            _subscriber->connect(_pubBrokerEndpoint);
            _subscriber->set(zmq::sockopt::subscribe, "");

            _isBrokerActive = true;

            if (debugEnabled()) {
              std::cout << "[App] Broker started (server mode) pub=" << _pubBrokerEndpoint
                    << " sub=" << _subBrokerEndpoint << std::endl;
            }

            _proxyThread = std::thread([this]() {
                try {
                    zmq::proxy(zmq::socket_ref(*_xsubSocket), zmq::socket_ref(*_xpubSocket));
                } catch (const zmq::error_t& e) {
                    if (e.num() != ETERM && _isBrokerActive) {
                        std::cerr << "Proxy error: " << e.what() << std::endl;
                    }
                }
            });
        } catch (const zmq::error_t& e) {
            std::cerr << "Failed to setup server message broker: " << e.what()
                      << " (Bind endpoints: " << _pubBrokerEndpoint << ", " << _subBrokerEndpoint << ")" << std::endl;
            cleanupMessageBroker();
        }
    } else {
        try {
            _publisher = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::pub);
            _subscriber = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::sub);

            _publisher->connect(_subBrokerEndpoint);
            _subscriber->connect(_pubBrokerEndpoint);
            _subscriber->set(zmq::sockopt::subscribe, "");

            _isBrokerActive = true;

            if (debugEnabled()) {
              std::cout << "[App] Broker connected (client mode) pub=" << _pubBrokerEndpoint
                    << " sub=" << _subBrokerEndpoint << std::endl;
            }
        } catch (const zmq::error_t& e) {
            std::cerr << "Failed to setup client message connections: " << e.what() << std::endl;
            cleanupMessageBroker();
        }
    }
}

void AApplication::cleanupMessageBroker() {
  _isBrokerActive = false;

  if (_publisher) {
    _publisher->close();
    _publisher.reset();
  }
  if (_subscriber) {
    _subscriber->close();
    _subscriber.reset();
  }

  if (_xpubSocket) {
    _xpubSocket->close();
    _xpubSocket.reset();
  }
  if (_xsubSocket) {
    _xsubSocket->close();
    _xsubSocket.reset();
  }

  if (_proxyThread.joinable()) {
    _proxyThread.join();
  }
}

void AApplication::addModule(const std::string &modulePath, const std::string &pubEndpoint, const std::string &subEndpoint) {
  if (debugEnabled()) {
    std::cout << "[App] Loading module: " << modulePath << " pub=" << pubEndpoint << " sub=" << subEndpoint << std::endl;
  }
  try {
    _modules.push_back(_modulesManager->loadModule(modulePath, pubEndpoint, subEndpoint));
  } catch (const std::exception& e) {
    std::cerr << "[App] Failed to load module '" << modulePath << "': " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "[App] Failed to load module '" << modulePath << "': unknown error" << std::endl;
  }
}

void AApplication::run() {
  _running = true;
  subscribe("ExitApplication", [this](const std::string&) {
    this->_running = false;
  });

  init();

  if (debugEnabled()) {
    std::cout << "[App] Starting " << _modules.size() << " modules" << std::endl;
  }

  for (const auto &module : _modules) {
    module->start();
  }

  auto lastProfileLog = std::chrono::steady_clock::now();
  double processMsTotal = 0.0;
  double loopMsTotal = 0.0;
  int profileSamples = 0;

  while (_running) {
    const auto processStart = std::chrono::steady_clock::now();
    processMessages();
    const auto processEnd = std::chrono::steady_clock::now();
    loop();
    const auto loopEnd = std::chrono::steady_clock::now();

    if (profileEnabled()) {
      processMsTotal += std::chrono::duration<double, std::milli>(processEnd - processStart).count();
      loopMsTotal += std::chrono::duration<double, std::milli>(loopEnd - processEnd).count();
      ++profileSamples;
      if (loopEnd - lastProfileLog >= std::chrono::seconds(1)) {
        const double sampleCount = profileSamples > 0 ? static_cast<double>(profileSamples) : 1.0;
        std::cout << "[PROFILE][App] process_avg_ms=" << (processMsTotal / sampleCount)
                  << " loop_avg_ms=" << (loopMsTotal / sampleCount)
                  << " modules=" << _modules.size()
                  << " samples=" << profileSamples << std::endl;
        processMsTotal = 0.0;
        loopMsTotal = 0.0;
        profileSamples = 0;
        lastProfileLog = loopEnd;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }

  for (const auto &module : _modules) {
    module->stop();
  }

  if (debugEnabled()) {
    std::cout << "[App] Shutdown complete" << std::endl;
  }

  cleanupMessageBroker();
}

void AApplication::sendMessage(const std::string& topic, const std::string& message) {
  if (!_publisher || !_isBrokerActive) {
    return;
  }

  std::string fullMessage = topic + " " + message;
  zmq::message_t zmqMessage(fullMessage.size());
  memcpy(zmqMessage.data(), fullMessage.c_str(), fullMessage.size());
  _publisher->send(zmqMessage, zmq::send_flags::none);

  if (debugEnabled()) {
    std::cout << "[Bus->] " << topic << " | " << truncatePayload(message) << std::endl;
  }
}

std::string AApplication::getMessage(const std::string& topic) {
  if (!_subscriber || !_isBrokerActive) {
    return "";
  }

  zmq::message_t zmqMessage;
  auto result = _subscriber->recv(zmqMessage, zmq::recv_flags::dontwait);

  if (!result) {
    return "";
  }

  std::string fullMessage(static_cast<char*>(zmqMessage.data()), zmqMessage.size());

  if (fullMessage.find(topic) == 0 && (fullMessage.length() == topic.length() || fullMessage[topic.length()] == ' ')) {
    size_t spacePos = fullMessage.find(' ');
    if (spacePos != std::string::npos && spacePos + 1 < fullMessage.size()) {
        return fullMessage.substr(spacePos + 1);
    }
    return "";
  }
  return "";
}

void AApplication::subscribe(const std::string& topic, MessageHandler handler) {
  if (!_subscriber || !_isBrokerActive) {
    return;
  }
  for (const auto& sub : _subscriptions) {
      if (sub.first == topic) {
          return;
      }
  }

  _subscriptions.push_back({topic, handler});
  _subscriber->set(zmq::sockopt::subscribe, topic);
}

void AApplication::unsubscribe(const std::string& topic) {
  if (!_subscriber || !_isBrokerActive) {
    return;
  }
  _subscriptions.erase(
      std::remove_if(_subscriptions.begin(), _subscriptions.end(),
          [&topic](const TopicSubscription& sub) {
              return sub.first == topic;
          }),
      _subscriptions.end());
  _subscriber->set(zmq::sockopt::unsubscribe, topic);
}

void AApplication::processMessages() {
  if (!_subscriber || !_isBrokerActive) {
    return;
  }

  int messagesProcessed = 0;
  const int maxMessagesPerLoop = 256;

  while (messagesProcessed < maxMessagesPerLoop) {
    zmq::message_t zmqMessage;
    auto result = _subscriber->recv(zmqMessage, zmq::recv_flags::dontwait);

    if (!result) {
      break;
    }

    messagesProcessed++;
    std::string fullMessage(static_cast<char*>(zmqMessage.data()), zmqMessage.size());

    std::string messageTopic = fullMessage;
    size_t spacePos = fullMessage.find(' ');
    if (spacePos != std::string::npos) {
      messageTopic = fullMessage.substr(0, spacePos);
    }

    if (debugEnabled()) {
        std::string payload = (spacePos != std::string::npos && spacePos + 1 < fullMessage.size())
                                  ? fullMessage.substr(spacePos + 1)
                                  : "";
        std::cout << "[Bus<-] " << messageTopic << " | " << truncatePayload(payload) << std::endl;
    }

    for (const auto& subscription : _subscriptions) {
      const std::string& topic = subscription.first;
      const MessageHandler& handler = subscription.second;

      if (messageTopic == topic) {
        std::string messageContent = "";
        if (spacePos != std::string::npos && spacePos + 1 < fullMessage.size()) {
          messageContent = fullMessage.substr(spacePos + 1);
        }
        try {
          handler(messageContent);
        } catch (const std::exception& e) {
          std::cerr << "[App] Handler error for topic '" << topic << "': " << e.what() << std::endl;
        } catch (...) {
          std::cerr << "[App] Handler error for topic '" << topic << "': unknown error" << std::endl;
        }
        break;
      }
    }
  }
}

}  // namespace rtypeEngine
