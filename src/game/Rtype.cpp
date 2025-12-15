#include "Rtype.hpp"
#include "modulesManager/ModulesManager.hpp"
#include <SFML/Window.hpp>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

namespace rtypeGame {
Rtype::Rtype(const std::string &endpoint, const std::vector<std::string> &args)
    : AApplication() {
  _endpoint = endpoint;
  _args = args;

  // Every Rtype instance (Client OR Server) needs its own internal ZMQ bus.
  // We act as "Server" of our own bus.
  bool isBusServer = true;
  std::string busEndpoint = _endpoint;

  // If we are a client, we must NOT bind to the same ports as the game server
  // (if running locally). Use ephemeral ports (0) to let the OS assign free
  // ports for our internal bus.
  if (!_args.empty() && _args[0] == "client") {
    size_t colon = _endpoint.find_last_of(':');
    if (colon != std::string::npos) {
      busEndpoint = _endpoint.substr(0, colon) + ":0";
    } else {
      busEndpoint = "127.0.0.1:0";
    }
  }

  // AApplication (base class) needs to know its role to set up ZMQ correctly
  this->setupBroker(busEndpoint, isBusServer);
}

void Rtype::init() {
  this->_modulesManager = std::make_shared<rtypeGame::ModulesManager>();

  // Use the broker endpoints setup in AApplication::setupBroker for modules
  std::string modulePubEndpoint = this->_pubBrokerEndpoint;
  std::string moduleSubEndpoint = this->_subBrokerEndpoint;

  try {
    bool isServer = (!_args.empty() && _args[0] == "server");

#ifdef _WIN32
    this->addModule("LuaECSManager.dll", modulePubEndpoint, moduleSubEndpoint);
    if (!isServer) {
      this->addModule("GLEWSFMLRenderer.dll", modulePubEndpoint,
                      moduleSubEndpoint);
      this->addModule("SFMLWindowManager.dll", modulePubEndpoint,
                      moduleSubEndpoint);
    }
    this->addModule("BulletPhysicEngine.dll", modulePubEndpoint,
                    moduleSubEndpoint);
    this->addModule("ECSSavesManager.dll", modulePubEndpoint,
                    moduleSubEndpoint);
    this->addModule("NetworkManager.dll", modulePubEndpoint, moduleSubEndpoint);
#else
    this->addModule("LuaECSManager.so", modulePubEndpoint, moduleSubEndpoint);
    if (!isServer) {
      this->addModule("GLEWSFMLRenderer.so", modulePubEndpoint,
                      moduleSubEndpoint);
      this->addModule("SFMLWindowManager.so", modulePubEndpoint,
                      moduleSubEndpoint);
    }
    this->addModule("BulletPhysicEngine.so", modulePubEndpoint,
                    moduleSubEndpoint);
    this->addModule("ECSSavesManager.so", modulePubEndpoint, moduleSubEndpoint);
    this->addModule("NetworkManager.so", modulePubEndpoint, moduleSubEndpoint);
#endif
  } catch (const std::exception &e) {
    std::cerr << "Failed to load a module: " << e.what() << std::endl;
    throw;
  }

  // Subscribe to network status and error messages
  subscribe("NetworkStatus", [this](const std::string &payload) {
    std::cout << "[NetworkStatus] " << payload << std::endl;
    // Client-side: If connected, send an initial CLIENT_HELLO
    if (_args[0] == "client" &&
        payload.find("Connected:") != std::string::npos) {
      std::cout << "[Client] Sending CLIENT_HELLO to server." << std::endl;
      sendMessage("RequestNetworkSend", "CLIENT_HELLO");
    }
  });

  subscribe("NetworkError", [this](const std::string &payload) {
    std::cerr << "[NetworkError] " << payload << std::endl;
  });

  // Subscribe to incoming network messages
  subscribe("NetworkMessage", [this](const std::string &payload) {
    std::string actualMessageType;
    std::string actualContent;
    uint32_t parsedClientId = 0;

    std::stringstream ss(payload);
    std::string firstWord;
    ss >> firstWord;

    if (!firstWord.empty() && std::isdigit(firstWord[0])) {
      try {
        parsedClientId = std::stoul(firstWord);
        if (ss >> actualMessageType) {
          std::getline(ss, actualContent);
          actualContent =
              actualContent.substr(actualContent.find_first_not_of(' ', 0));
        } else {
          actualMessageType = "";
          actualContent = "";
        }
      } catch (...) {
        actualMessageType = payload;
        parsedClientId = 0;
        actualContent = "";
      }
    } else {
      actualMessageType = firstWord;
      std::getline(ss, actualContent);
      actualContent =
          actualContent.substr(actualContent.find_first_not_of(' ', 0));
    }

    if (actualMessageType.empty() && !payload.empty()) {
      actualMessageType = payload;
    }
    if (!actualContent.empty() &&
        actualContent.find_first_not_of(' ') == std::string::npos) {
      actualContent = "";
    }

    if (actualMessageType == "CLIENT_HELLO") {
      if (_args[0] == "server") {
        std::cout << "[Server] Received CLIENT_HELLO from Client "
                  << parsedClientId << std::endl;
        sendMessage("RequestNetworkSendTo", std::to_string(parsedClientId) +
                                                " YOUR_ID " +
                                                std::to_string(parsedClientId));
      }
    } else if (actualMessageType == "YOUR_ID") {
      if (_args[0] == "client") {
        try {
          _myClientId = std::stoul(actualContent);
          std::cout << "[Client] My assigned ID is " << _myClientId
                    << std::endl;
        } catch (const std::exception &e) {
          std::cerr << "[Client] Error parsing YOUR_ID content: " << e.what()
                    << std::endl;
        }
      }
    } else if (actualMessageType == "PING") {
      if (_args[0] == "server") {
        std::cout << "[Server] Received PING from Client " << parsedClientId
                  << std::endl;
        sendMessage("RequestNetworkBroadcast",
                    "PONG " + std::to_string(parsedClientId));
      }
    } else if (actualMessageType == "PONG") {
      uint32_t pongClientId = 0;
      try {
        pongClientId = std::stoul(actualContent);
        std::cout << "[Client] Received PONG from Client " << pongClientId
                  << std::endl;
      } catch (...) {
        std::cout << "[Client] Received PONG (no valid client ID)" << std::endl;
      }
    }
  });
}

void Rtype::loop() {
  if (!_networkInitDone && !_args.empty()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    if (_args[0] == "server" && _args.size() >= 2) {
      std::cout << "[Game] Requesting Bind on port " << _args[1] << std::endl;
      sendMessage("RequestNetworkBind", _args[1]);
    } else if (_args[0] == "client" && _args.size() >= 3) {
      std::string connectPayload = _args[1] + " " + _args[2];
      std::cout << "[Game] Requesting Connect to " << connectPayload
                << std::endl;
      sendMessage("RequestNetworkConnect", connectPayload);
    }
    _networkInitDone = true;
  }

  // Ping logic for client
  if (_args[0] == "client") {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
      if (!_spacePressed) {
        if (_myClientId != 0) {
          std::cout << "[Client " << _myClientId << "] Sending PING"
                    << std::endl;
          sendMessage("RequestNetworkSend",
                      "PING " + std::to_string(_myClientId));
        } else {
          std::cout << "[Client] Sending PING (ID unknown yet)" << std::endl;
          sendMessage("RequestNetworkSend", "PING");
        }
        _spacePressed = true;
      }
    } else {
      _spacePressed = false;
    }
  }

  if (!_scriptsLoaded && _args.empty()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "Loading space-shooter game script..." << std::endl;
    sendMessage("LoadScript", "assets/scripts/space-shooter/Game.lua");
    _scriptsLoaded = true;
  }
}

} // namespace rtypeGame