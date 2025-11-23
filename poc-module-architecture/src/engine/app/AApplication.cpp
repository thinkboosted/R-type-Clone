

#include "AApplication.hpp"
#include <chrono>
#include <iostream>

namespace rtypeEngine {

AApplication::AApplication()
    : _zmqContext(1),
      _xpubSocket(nullptr),
      _xsubSocket(nullptr),
      _brokerInitialized(false) {}

AApplication::~AApplication() {
  cleanupMessageBroker();
}

void AApplication::initializeMessageBroker() {
  if (_brokerInitialized) {
    return;
  }

  try {
    _xpubSocket = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::xpub);
    _xsubSocket = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::xsub);
    _subscriber = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::sub);
    _publisher = std::make_unique<zmq::socket_t>(_zmqContext, zmq::socket_type::pub);

    std::string pubEndpoint = _endpoint;
    std::string subEndpoint = _endpoint;

    size_t colonPos = _endpoint.find_last_of(':');
    if (colonPos != std::string::npos) {
      std::string base = _endpoint.substr(0, colonPos);
      int port = std::stoi(_endpoint.substr(colonPos + 1));
      pubEndpoint = base + ":" + std::to_string(port);
      subEndpoint = base + ":" + std::to_string(port + 1);
    }

    if (pubEndpoint.find("tcp://") != 0 && pubEndpoint.find("ipc://") != 0) {
      pubEndpoint = "tcp://" + pubEndpoint;
    }
    if (subEndpoint.find("tcp://") != 0 && subEndpoint.find("ipc://") != 0) {
      subEndpoint = "tcp://" + subEndpoint;
    }

    _xpubSocket->bind(pubEndpoint);
    _xsubSocket->bind(subEndpoint);

    _publisher->connect(subEndpoint);
    _subscriber->connect(pubEndpoint);

    _brokerInitialized = true;

    _proxyThread = std::thread([this]() {
      try {
        zmq::proxy(zmq::socket_ref(*_xsubSocket), zmq::socket_ref(*_xpubSocket));
      } catch (const zmq::error_t& e) {
        if (e.num() != ETERM && _brokerInitialized) {
          std::cerr << "Proxy error: " << e.what() << std::endl;
        }
      }
    });
  } catch (const zmq::error_t& e) {
    std::cerr << "Failed to bind message broker: " << e.what() << std::endl;
    throw;
  }
}

void AApplication::cleanupMessageBroker() {
  _brokerInitialized = false;

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
  }
  if (_xsubSocket) {
    _xsubSocket->close();
  }

  if (_proxyThread.joinable()) {
    _proxyThread.join();
  }

  _xpubSocket.reset();
  _xsubSocket.reset();
}

void AApplication::addModule(const std::string &modulePath, const std::string &pubEndpoint, const std::string &subEndpoint) {
  _modules.push_back(_modulesManager->loadModule(modulePath, pubEndpoint, subEndpoint));
}

void AApplication::run() {
  initializeMessageBroker();

  _running = true;
  subscribe("ExitApplication", [this](const std::string&) {
    this->_running = false;
  }); // Par défaut pour toutes les apps on subscribe à ExitApplication

  init();

  for (const auto &module : _modules) {
    try {
      module->start();
    } catch (const std::exception &e) {
      std::cerr << "Module start failed: " << e.what() << std::endl;
    }
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
  if (!_publisher) {
    return;
  }

  std::string fullMessage = topic + " " + message;
  zmq::message_t zmqMessage(fullMessage.size());
  memcpy(zmqMessage.data(), fullMessage.c_str(), fullMessage.size());
  _publisher->send(zmqMessage, zmq::send_flags::none);
}

std::string AApplication::getMessage(const std::string& topic) {
  if (!_subscriber) {
    return "";
  }

  zmq::message_t zmqMessage;
  auto result = _subscriber->recv(zmqMessage, zmq::recv_flags::dontwait);

  if (!result) {
    return "";
  }

  std::string fullMessage(static_cast<char*>(zmqMessage.data()), zmqMessage.size());

  if (fullMessage.find(topic) != 0) {
    return "";
  }

  size_t spacePos = fullMessage.find(' ');
  if (spacePos != std::string::npos && spacePos + 1 < fullMessage.size()) {
    return fullMessage.substr(spacePos + 1);
  }

  return fullMessage;
}

void AApplication::subscribe(const std::string& topic, MessageHandler handler) {
  if (_subscriber) {
    _subscriber->set(zmq::sockopt::subscribe, topic);
  }
  _subscriptions.push_back({topic, handler});
}

void AApplication::unsubscribe(const std::string& topic) {
  _subscriptions.erase(
      std::remove_if(_subscriptions.begin(), _subscriptions.end(),
          [&topic](const TopicSubscription& sub) {
              return sub.first == topic;
          }),
      _subscriptions.end());
  if (_subscriber) {
    _subscriber->set(zmq::sockopt::unsubscribe, topic);
  }
}

void AApplication::processMessages() {
  if (!_subscriber) {
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

      if (fullMessage.find(topic) == 0) {
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
