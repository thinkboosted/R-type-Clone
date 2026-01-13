#include "AApplication.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept> // For std::stoi
#include <string>

#include "../modulesManager/ModulesManager.hpp"

namespace {
bool debugEnabled() {
  static bool enabled = (std::getenv("RTYPE_DEBUG") != nullptr);
  return enabled;
}

std::string truncatePayload(const std::string& msg, std::size_t limit = 200) {
  if (msg.size() <= limit) {
    return msg;
  }
  return msg.substr(0, limit) + "...";
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
      _isServerMode(false) {
  // Ensure modules manager is ready before any module load
  _modulesManager = std::make_shared<rtypeGame::ModulesManager>();
}

AApplication::~AApplication() {
  cleanupMessageBroker();
}

void AApplication::setupBroker(const std::string& baseEndpoint, bool isServer) {
    _isServerMode = isServer;

    // ═══════════════════════════════════════════════════════════════════════════
    // DETECT LOCAL MODE (inproc:// - In-Process, No Network)
    // ═══════════════════════════════════════════════════════════════════════════
    if (baseEndpoint.empty() ||
        baseEndpoint == "local" ||
        baseEndpoint.find("inproc://") == 0) {
        _isLocalMode = true;
        _pubBrokerEndpoint = "inproc://game_bus_pub";
        _subBrokerEndpoint = "inproc://game_bus_sub";

        if (debugEnabled()) {
            std::cout << "[App] LOCAL MODE detected (inproc://)" << std::endl;
        }
    }
    // Network mode (tcp:// or ipc://)
    else if (baseEndpoint.find(":*") != std::string::npos) {
        _pubBrokerEndpoint = baseEndpoint;
        _subBrokerEndpoint = baseEndpoint; // Will bind to a new random port
    } else {
        size_t colonPos = baseEndpoint.find_last_of(':');
        if (colonPos != std::string::npos) {
            std::string base = baseEndpoint.substr(0, colonPos);
            int port = 0;
            try {
                port = std::stoi(baseEndpoint.substr(colonPos + 1));
            } catch (const std::exception& e) {
                std::cerr << "Invalid port in baseEndpoint: " << e.what() << std::endl;
                throw;
            }
            _pubBrokerEndpoint = base + ":" + std::to_string(port);
            _subBrokerEndpoint = base + ":" + std::to_string(port + 1);
        } else {
            _pubBrokerEndpoint = baseEndpoint;
            _subBrokerEndpoint = baseEndpoint;
        }
    }

    // Add protocol prefix if not already present (skip for local mode)
    if (!_isLocalMode) {
        if (_pubBrokerEndpoint.find("tcp://") != 0 && _pubBrokerEndpoint.find("ipc://") != 0 && _pubBrokerEndpoint.find("inproc://") != 0) {
            _pubBrokerEndpoint = "tcp://" + _pubBrokerEndpoint;
        }
        if (_subBrokerEndpoint.find("tcp://") != 0 && _subBrokerEndpoint.find("ipc://") != 0 && _subBrokerEndpoint.find("inproc://") != 0) {
            _subBrokerEndpoint = "tcp://" + _subBrokerEndpoint;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // BROKER SETUP (Local or Server) - Enables Forwarding
    // ═══════════════════════════════════════════════════════════════════════════
    if (_isLocalMode || _isServerMode) {
        try {
            // Use XPUB/XSUB for the broker (allows forwarding)
            _xpubSocket = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::xpub);
            _xsubSocket = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::xsub);
            _publisher = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::pub);
            _subscriber = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::sub);

            // Bind the Broker sockets
            _xpubSocket->bind(_pubBrokerEndpoint); // Modules SUBSCRIBE here
            _xsubSocket->bind(_subBrokerEndpoint); // Modules PUBLISH here

            // Update endpoints if wildcard was used (Server mode)
            if (!_isLocalMode) {
                if (_pubBrokerEndpoint.find(":*") != std::string::npos) {
                    _pubBrokerEndpoint = _xpubSocket->get(zmq::sockopt::last_endpoint);
                }
                if (_subBrokerEndpoint.find(":*") != std::string::npos) {
                    _subBrokerEndpoint = _xsubSocket->get(zmq::sockopt::last_endpoint);
                }
            }

            // Connect App's sockets to the Broker
            _publisher->connect(_subBrokerEndpoint);
            _subscriber->connect(_pubBrokerEndpoint);
            _subscriber->set(zmq::sockopt::subscribe, "");

            _isBrokerActive = true;

            if (debugEnabled()) {
                std::cout << "[App] Broker started (Proxy Mode) pub=" << _pubBrokerEndpoint
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
            std::cerr << "Failed to setup message broker: " << e.what() << std::endl;
            throw;
        }
    }
    // ═══════════════════════════════════════════════════════════════════════════
    // CLIENT MODE (No Broker, just connect)
    // ═══════════════════════════════════════════════════════════════════════════
    else {
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
            throw;
        }
    }
}

void AApplication::cleanupMessageBroker() {
  _isBrokerActive = false;

  if (_publisher) {
    try { _publisher->set(zmq::sockopt::linger, 0); } catch (...) {}
    try { _publisher->close(); } catch (...) {}
    _publisher.reset();
  }
  if (_subscriber) {
    try { _subscriber->set(zmq::sockopt::linger, 0); } catch (...) {}
    try { _subscriber->close(); } catch (...) {}
    _subscriber.reset();
  }

  if (_xpubSocket) {
    try { _xpubSocket->set(zmq::sockopt::linger, 0); } catch (...) {}
    try { _xpubSocket->close(); } catch (...) {}
    _xpubSocket.reset();
  }
  if (_xsubSocket) {
    try { _xsubSocket->set(zmq::sockopt::linger, 0); } catch (...) {}
    try { _xsubSocket->close(); } catch (...) {}
    _xsubSocket.reset();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  if (_proxyThread.joinable()) {
    try {
      _proxyThread.join();
    } catch (const std::exception& e) {
      if (debugEnabled()) {
        std::cerr << "[App] Error joining proxy thread: " << e.what() << std::endl;
      }
    }
  }
}

void AApplication::addModule(const std::string &modulePath, const std::string &pubEndpoint, const std::string &subEndpoint, void* sharedZmqContext) {
  if (!_modulesManager) {
    throw std::runtime_error("[App] ModulesManager not initialized!");
  }
  if (debugEnabled()) {
    std::cout << "[App] Loading module: " << modulePath << " pub=" << pubEndpoint << " sub=" << subEndpoint << std::endl;
  }
  _modules.push_back(_modulesManager->loadModule(modulePath, pubEndpoint, subEndpoint, sharedZmqContext));
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

  while (_running) {
    processMessages();
    loop();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
    bool isBinary = (topic == "ImageRendered" || message.size() > 200);
    std::cout << "[Bus->] " << topic << " | ";
    if (isBinary) {
        std::cout << "[Binary Data / Payload too large: " << message.size() << " bytes]";
    } else {
        std::cout << message;
    }
    std::cout << std::endl;
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
  const int maxMessagesPerLoop = 100;

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
        bool isBinary = (messageTopic == "ImageRendered" || payload.size() > 200);
        std::cout << "[Bus<-] " << messageTopic << " | ";
        if (isBinary) {
            std::cout << "[Binary Data / Payload too large: " << payload.size() << " bytes]";
        } else {
            std::cout << payload;
        }
        std::cout << std::endl;
    }

    for (const auto& subscription : _subscriptions) {
      const std::string& topic = subscription.first;
      const MessageHandler& handler = subscription.second;

      if (messageTopic == topic) {
        std::string messageContent = "";
        if (spacePos != std::string::npos && spacePos + 1 < fullMessage.size()) {
          messageContent = fullMessage.substr(spacePos + 1);
        }
        handler(messageContent);
        break;
      }
    }
  }
}

std::string AApplication::getEndpoint(const std::string& type, bool useLocal) {
    // ═══════════════════════════════════════════════════════════════════════════
    // ENDPOINT FACTORY - Returns appropriate endpoint based on mode
    // ═══════════════════════════════════════════════════════════════════════════
    // Local mode:  inproc://game_bus_{type}
    // Remote mode: tcp://0.0.0.0:PORT or current endpoints
    // ═══════════════════════════════════════════════════════════════════════════

    if (_isLocalMode || useLocal) {
        return "inproc://game_bus_" + type;
    }

    // Return configured endpoints
    if (type == "pub") {
        return _pubBrokerEndpoint;
    } else if (type == "sub") {
        return _subBrokerEndpoint;
    }

    // Fallback (should never happen)
    return "tcp://127.0.0.1:5555";
}

}  // namespace rtypeEngine