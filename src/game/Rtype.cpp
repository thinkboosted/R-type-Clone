#include "Rtype.hpp"
#include "modulesManager/ModulesManager.hpp"
#include <string>
#include <iostream>
#include <thread>
#include <chrono>

namespace rtypeGame {
Rtype::Rtype(const std::string &endpoint, const std::vector<std::string>& args) : AApplication() {
  _endpoint = endpoint;
  _args = args;
}

void Rtype::init() {
  this->_modulesManager = std::make_shared<rtypeGame::ModulesManager>();

  size_t colonPos = _endpoint.find_last_of(':');
  std::string pubEndpoint = _endpoint;
  std::string subEndpoint = _endpoint;

  if (colonPos != std::string::npos) {
    std::string base = _endpoint.substr(0, colonPos);
    int port = std::stoi(_endpoint.substr(colonPos + 1));
    pubEndpoint = base + ":" + std::to_string(port);
    subEndpoint = base + ":" + std::to_string(port + 1);
  }

  try {
#ifdef _WIN32
  this->addModule("LuaECSManager.dll", pubEndpoint, subEndpoint);
  this->addModule("GLEWSFMLRenderer.dll", pubEndpoint, subEndpoint);
  this->addModule("SFMLWindowManager.dll", pubEndpoint, subEndpoint);
  this->addModule("BulletPhysicEngine.dll", pubEndpoint, subEndpoint);
  this->addModule("ECSSavesManager.dll", pubEndpoint, subEndpoint);
  this->addModule("NetworkManager.dll", pubEndpoint, subEndpoint);
#else
  this->addModule("LuaECSManager.so", pubEndpoint, subEndpoint);
  this->addModule("GLEWSFMLRenderer.so", pubEndpoint, subEndpoint);
  this->addModule("SFMLWindowManager.so", pubEndpoint, subEndpoint);
  this->addModule("BulletPhysicEngine.so", pubEndpoint, subEndpoint);
  this->addModule("ECSSavesManager.so", pubEndpoint, subEndpoint);
  this->addModule("NetworkManager.so", pubEndpoint, subEndpoint);
#endif
  } catch (const std::exception& e) {
    std::cerr << "Failed to load a module: " << e.what() << std::endl;
    throw;
  }

  // Subscribe to network status and error messages
  subscribe("NetworkStatus", [this](const std::string& payload) {
    std::cout << "[NetworkStatus] " << payload << std::endl;
  });

  subscribe("NetworkError", [this](const std::string& payload) {
    std::cerr << "[NetworkError] " << payload << std::endl;
  });

  // Subscribe to incoming network messages
  subscribe("NetworkMessage", [this](const std::string& payload) {
    std::cout << "[Incoming NetworkMessage] " << payload << std::endl;
  });
}

void Rtype::loop() {
    if (!_networkInitDone && !_args.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Wait for modules to be ready

        if (_args[0] == "server" && _args.size() >= 2) {
            std::cout << "[Game] Requesting Bind on port " << _args[1] << std::endl;
            sendMessage("RequestNetworkBind", _args[1]);
        } else if (_args[0] == "client" && _args.size() >= 3) {
            std::string connectPayload = _args[1] + " " + _args[2];
            std::cout << "[Game] Requesting Connect to " << connectPayload << std::endl;
            sendMessage("RequestNetworkConnect", connectPayload);

            // Send a test message shortly after
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            sendMessage("RequestNetworkSend", "HelloFromClient");
        }
        _networkInitDone = true;
    }

    if (!_scriptsLoaded && _args.empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      std::cout << "Loading space-shooter game script..." << std::endl;
      sendMessage("LoadScript", "assets/scripts/space-shooter/Game.lua");
      _scriptsLoaded = true;
    }
}

}  // namespace rtypeGame
