#include "AApplication.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept> // For std::stoi

namespace rtypeEngine {

// AApplication Constructor: Sockets are now initialized in setupBroker
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

// New method to configure and initialize the ZMQ broker/connections based on role
void AApplication::setupBroker(const std::string& baseEndpoint, bool isServer) {
    _isServerMode = isServer;

    // Calculate actual ZMQ endpoints
    size_t colonPos = baseEndpoint.find_last_of(':');
    if (colonPos != std::string::npos) {
        std::string base = baseEndpoint.substr(0, colonPos);
        int port = 0;
        try {
            port = std::stoi(baseEndpoint.substr(colonPos + 1));
        } catch (const std::exception& e) {
            std::cerr << "Invalid port in baseEndpoint: " << e.what() << std::endl;
            throw; // Re-throw to indicate critical error
        }
        _pubBrokerEndpoint = base + ":" + std::to_string(port);
        _subBrokerEndpoint = base + ":" + std::to_string(port + 1);
    } else {
        // Assume inproc:// or ipc:// if no port specified
        _pubBrokerEndpoint = baseEndpoint;
        _subBrokerEndpoint = baseEndpoint; 
    }

    // Ensure endpoints have a valid protocol prefix
    if (_pubBrokerEndpoint.find("tcp://") != 0 && _pubBrokerEndpoint.find("ipc://") != 0 && _pubBrokerEndpoint.find("inproc://") != 0) {
        _pubBrokerEndpoint = "tcp://" + _pubBrokerEndpoint;
    }
    if (_subBrokerEndpoint.find("tcp://") != 0 && _subBrokerEndpoint.find("ipc://") != 0 && _subBrokerEndpoint.find("inproc://") != 0) {
        _subBrokerEndpoint = "tcp://" + _subBrokerEndpoint;
    }

    if (_isServerMode) {
        // Server Mode: This AApplication instance acts as the central broker
        try {
            _xpubSocket = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::xpub);
            _xsubSocket = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::xsub);
            _publisher = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::pub);
            _subscriber = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::sub);

            // Handle ephemeral ports (port 0)
            if (_pubBrokerEndpoint.find(":0") != std::string::npos) {
                _xpubSocket->bind("tcp://127.0.0.1:*");
                _pubBrokerEndpoint = _xpubSocket->get(zmq::sockopt::last_endpoint);
            } else {
                _xpubSocket->bind(_pubBrokerEndpoint);
            }

            if (_subBrokerEndpoint.find(":1") != std::string::npos && baseEndpoint.find(":0") != std::string::npos) {
                 // If base was port 0, sub endpoint calculation +1 is invalid/meaningless before bind. 
                 // Just bind sub to random port too.
                 _xsubSocket->bind("tcp://127.0.0.1:*");
                 _subBrokerEndpoint = _xsubSocket->get(zmq::sockopt::last_endpoint);
            } else {
                _xsubSocket->bind(_subBrokerEndpoint);
            }

            // AApplication's own publisher/subscriber connect to its internal broker
            _publisher->connect(_subBrokerEndpoint);
            _subscriber->connect(_pubBrokerEndpoint);
            _subscriber->set(zmq::sockopt::subscribe, ""); // Subscribe to all for internal processing

            _isBrokerActive = true;

            _proxyThread = std::thread([this]() {
                try {
                    zmq::proxy(zmq::socket_ref(*_xsubSocket), zmq::socket_ref(*_xpubSocket));
                } catch (const zmq::error_t& e) {
                    // ETERM means context was terminated, which is expected during shutdown
                    if (e.num() != ETERM && _isBrokerActive) {
                        std::cerr << "Proxy error: " << e.what() << std::endl;
                    }
                }
            });
        } catch (const zmq::error_t& e) {
            std::cerr << "Failed to setup server message broker: " << e.what() << std::endl;
            throw;
        }
    } else {
        // Client Mode: This AApplication instance connects to an external broker (the server's)
        try {
            _publisher = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::pub);
            _subscriber = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::sub);

            _publisher->connect(_subBrokerEndpoint); // Connect to server's XSUB
            _subscriber->connect(_pubBrokerEndpoint); // Connect to server's XPUB
            _subscriber->set(zmq::sockopt::subscribe, ""); // Subscribe to all for client-side processing

            _isBrokerActive = true; // Broker is active via connection
        } catch (const zmq::error_t& e) {
            std::cerr << "Failed to setup client message connections: " << e.what() << std::endl;
            throw;
        }
    }
}

// cleanupMessageBroker: Adjusted to handle potentially null _xpubSocket/_xsubSocket
void AApplication::cleanupMessageBroker() {
  _isBrokerActive = false;

  // Close publisher and subscriber always
  if (_publisher) {
    _publisher->close();
    _publisher.reset();
  }
  if (_subscriber) {
    _subscriber->close();
    _subscriber.reset();
  }

  // Only close xpub/xsub if they exist (server mode)
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
  _modules.push_back(_modulesManager->loadModule(modulePath, pubEndpoint, subEndpoint));
}

// run method: No longer calls initializeMessageBroker()
void AApplication::run() {
  // initializeMessageBroker(); // Removed - setupBroker handles this now

  _running = true;
  subscribe("ExitApplication", [this](const std::string&) {
    this->_running = false;
  }); // By default for all apps we subscribe to ExitApplication

  init();

  for (const auto &module : _modules) {
    // Module start takes pubEndpoint and subEndpoint for the bus
    // These should be the _pubBrokerEndpoint and _subBrokerEndpoint
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

  cleanupMessageBroker();
}

// sendMessage, getMessage, subscribe, unsubscribe, processMessages:
// Added checks for _publisher/_subscriber validity
void AApplication::sendMessage(const std::string& topic, const std::string& message) {
  if (!_publisher || !_isBrokerActive) {
    std::cerr << "Attempt to send message without active publisher." << std::endl;
    return;
  }

  std::string fullMessage = topic + " " + message;
  zmq::message_t zmqMessage(fullMessage.size());
  memcpy(zmqMessage.data(), fullMessage.c_str(), fullMessage.size());
  _publisher->send(zmqMessage, zmq::send_flags::none);
}

std::string AApplication::getMessage(const std::string& topic) {
  if (!_subscriber || !_isBrokerActive) {
    // std::cerr << "Attempt to get message without active subscriber." << std::endl;
    return "";
  }

  zmq::message_t zmqMessage;
  auto result = _subscriber->recv(zmqMessage, zmq::recv_flags::dontwait);

  if (!result) {
    return "";
  }

  std::string fullMessage(static_cast<char*>(zmqMessage.data()), zmqMessage.size());

  // Check if the message actually matches the expected topic
  // Note: This logic assumes topic is at the beginning, then a space.
  if (fullMessage.find(topic) == 0 && (fullMessage.length() == topic.length() || fullMessage[topic.length()] == ' ')) {
    size_t spacePos = fullMessage.find(' ');
    if (spacePos != std::string::npos && spacePos + 1 < fullMessage.size()) {
        return fullMessage.substr(spacePos + 1);
    }
    return ""; // Topic match but no payload
  }
  return ""; // Topic doesn't match
}

void AApplication::subscribe(const std::string& topic, MessageHandler handler) {
  if (!_subscriber || !_isBrokerActive) {
    std::cerr << "Attempt to subscribe without active subscriber." << std::endl;
    return;
  }
  // Check if already subscribed to prevent duplicate handlers
  for (const auto& sub : _subscriptions) {
      if (sub.first == topic) {
          // std::cerr << "Already subscribed to topic: " << topic << std::endl;
          return;
      }
  }

  _subscriptions.push_back({topic, handler});
  _subscriber->set(zmq::sockopt::subscribe, topic);
}

void AApplication::unsubscribe(const std::string& topic) {
  if (!_subscriber || !_isBrokerActive) {
    std::cerr << "Attempt to unsubscribe without active subscriber." << std::endl;
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
    // Use ZMQ_DONTWAIT to avoid blocking
    auto result = _subscriber->recv(zmqMessage, zmq::recv_flags::dontwait);

    if (!result) {
      break; // No more messages
    }

    messagesProcessed++;
    std::string fullMessage(static_cast<char*>(zmqMessage.data()), zmqMessage.size());

    // Extract topic from the message
    std::string messageTopic = fullMessage;
    size_t spacePos = fullMessage.find(' ');
    if (spacePos != std::string::npos) {
      messageTopic = fullMessage.substr(0, spacePos);
    }

    for (const auto& subscription : _subscriptions) {
      const std::string& topic = subscription.first;
      const MessageHandler& handler = subscription.second;

      // Check if message topic matches subscription topic
      // This is a more robust check than just find() == 0,
      // as it handles cases where topic is a prefix of another word.
      if (messageTopic == topic) {
        std::string messageContent = "";
        if (spacePos != std::string::npos && spacePos + 1 < fullMessage.size()) {
          messageContent = fullMessage.substr(spacePos + 1);
        }
        handler(messageContent);
        // Only one handler per topic? Or all matching?
        // Assuming one handler per topic for now, or the first one.
        // If multiple subscriptions for the same topic, all would be called.
        // For current architecture, let's break after first match.
        break; 
      }
    }
  }
}

}  // namespace rtypeEngine