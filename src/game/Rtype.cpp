#include "Rtype.hpp"
#include "modulesManager/ModulesManager.hpp"
#include <string>
#include <iostream>

namespace rtypeGame {
Rtype::Rtype(const std::string &endpoint) : AApplication() {
  _endpoint = endpoint;
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
  this->addModule("GLEWRenderer.dll", pubEndpoint, subEndpoint);
  this->addModule("SFMLWindowManager.dll", pubEndpoint, subEndpoint);
  this->addModule("BulletPhysicEngine.dll", pubEndpoint, subEndpoint);
  this->addModule("ECSSavesManager.dll", pubEndpoint, subEndpoint);
#else
  this->addModule("LuaECSManager.so", pubEndpoint, subEndpoint);
  this->addModule("GLEWRenderer.so", pubEndpoint, subEndpoint);
  this->addModule("SFMLWindowManager.so", pubEndpoint, subEndpoint);
  this->addModule("BulletPhysicEngine.so", pubEndpoint, subEndpoint);
  this->addModule("ECSSavesManager.so", pubEndpoint, subEndpoint);
#endif
  } catch (const std::exception& e) {
    std::cerr << "Failed to load a module: " << e.what() << std::endl;
    throw;
  }
}

void Rtype::loop() {
    if (!_scriptsLoaded) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      sendMessage("LoadScript", "assets/scripts/space-shooter/Game.lua");
      _scriptsLoaded = true;
    }
}

}  // namespace rtypeGame
