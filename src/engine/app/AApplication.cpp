
#include "AApplication.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept> // For std::stoi

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

            _xpubSocket->bind(_pubBrokerEndpoint);
            _xsubSocket->bind(_subBrokerEndpoint);

            _publisher->connect(_subBrokerEndpoint);
            _subscriber->connect(_pubBrokerEndpoint);
            _subscriber->set(zmq::sockopt::subscribe, "");

            _isBrokerActive = true;

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
            std::cerr << "Failed to setup server message broker: " << e.what() << std::endl;
            throw;
        }
    } else {
        try {
            _publisher = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::pub);
            _subscriber = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::sub);

            _publisher->connect(_subBrokerEndpoint);
            _subscriber->connect(_pubBrokerEndpoint);
            _subscriber->set(zmq::sockopt::subscribe, "");

            _isBrokerActive = true;
        } catch (const zmq::error_t& e) {
            std::cerr << "Failed to setup client message connections: " << e.what() << std::endl;
            throw;
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
  _modules.push_back(_modulesManager->loadModule(modulePath, pubEndpoint, subEndpoint));
}

void AApplication::run() {
  _running = true;
  subscribe("ExitApplication", [this](const std::string&) {
    this->_running = false;
  });

  init();

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

}  // namespace rtypeEngine
