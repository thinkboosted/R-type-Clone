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

  if (!_args.empty()) {
      size_t colon = _endpoint.find_last_of(':');
      std::string ip = (colon != std::string::npos) ? _endpoint.substr(0, colon) : "127.0.0.1";

      if (_args[0] == "server" && _args.size() >= 2) {
          // Server uses the specific port provided in args
          busEndpoint = ip + ":" + _args[1];
      } else if (_args[0] == "client") {
          // Client uses a random port (wildcard) to avoid conflicts
          busEndpoint = "tcp://" + ip + ":*";
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

  // Subscribe to network status and error messages for debugging
  subscribe("NetworkStatus", [this](const std::string &payload) {
    std::cout << "[NetworkStatus] " << payload << std::endl;
  });

  subscribe("NetworkError", [this](const std::string &payload) {
    std::cerr << "[NetworkError] " << payload << std::endl;
  });

  // Log incoming network messages (useful for debugging multiplayer sync)
  subscribe("NetworkMessage", [this](const std::string& payload) {
    // Uncomment the line below to see all network traffic in console
    // std::cout << "[Network] " << payload << std::endl;
  });
}

void Rtype::loop() {
    // 1. Network Initialization (One time)
    if (!_networkInitDone && !_args.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        if (_args[0] == "server" && _args.size() >= 2) {
            std::cout << "[Game] Requesting Bind on port " << _args[1] << std::endl;
            sendMessage("RequestNetworkBind", _args[1]);
        } else if (_args[0] == "client" && _args.size() >= 3) {
            std::string connectPayload = _args[1] + " " + _args[2];
            std::cout << "[Game] Requesting Connect to " << connectPayload << std::endl;
            sendMessage("RequestNetworkConnect", connectPayload);
        }
        _networkInitDone = true;
    }

    if (!_scriptsLoaded) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "Loading space-shooter game script..." << std::endl;
        sendMessage("LoadScript", "assets/scripts/space-shooter/Game.lua");
        _scriptsLoaded = true;
    }
}

} // namespace rtypeGame
