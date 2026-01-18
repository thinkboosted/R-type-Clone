#include "PlatformerGame.hpp"
#include "modulesManager/ModulesManager.hpp"
#include <iostream>
#include <thread>
#include <chrono>

namespace platformerGame {

PlatformerGame::PlatformerGame() : AApplication() {
    this->_modulesManager = std::make_shared<rtypeGame::ModulesManager>();
    setupBroker("127.0.0.1:*", true);
}

void PlatformerGame::loadModule(const std::string &moduleName) {
    std::string path = moduleName;
#ifdef _WIN32
    path += ".dll";
#else
    path += ".so";
#endif
    this->addModule(path, this->_pubBrokerEndpoint, this->_subBrokerEndpoint);
}

void PlatformerGame::init() {
    subscribe("LevelComplete", [this](const std::string &payload) {
        std::cout << "[Platformer] Level completed! Loading: " << payload << std::endl;
        sendMessage("LoadScript", payload);
    });

    subscribe("PlayerDied", [this](const std::string &payload) {
        std::cout << "[Platformer] Player died - respawning at checkpoint" << std::endl;
    });

    std::cout << "[Platformer] Initialized" << std::endl;
}

void PlatformerGame::loop() {
    if (!_scriptsLoaded) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "[Platformer] Loading game script..." << std::endl;
        sendMessage("LoadScript", "assets/scripts/platformer/Main.lua");
        _scriptsLoaded = true;
    }
}

} // namespace platformerGame
