#include "PlatformerGame.hpp"
#include "modulesManager/ModulesManager.hpp"
#include <iostream>
#include <thread>
#include <chrono>

namespace platformerGame {

PlatformerGame::PlatformerGame(PlatformerMode mode, const std::string &serverIp,
                               int serverPort)
    : AApplication(), _mode(mode), _serverIp(serverIp), _serverPort(serverPort) {
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

    if (_mode == PlatformerMode::Host) {
        sendMessage("SetPlatformerMode", "HOST");
    } else if (_mode == PlatformerMode::Client) {
        sendMessage("SetPlatformerMode", "CLIENT");
    } else {
        sendMessage("SetPlatformerMode", "SOLO");
    }

    std::cout << "[Platformer] Initialized" << std::endl;
}

void PlatformerGame::loop() {
    if (!_networkInitDone) {
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
        if (_mode == PlatformerMode::Host) {
            std::cout << "[Platformer] Host binding on port " << _serverPort << std::endl;
            sendMessage("RequestNetworkBind", std::to_string(_serverPort));
        } else if (_mode == PlatformerMode::Client) {
            std::cout << "[Platformer] Connecting to " << _serverIp << ":" << _serverPort << std::endl;
            sendMessage("RequestNetworkConnect", _serverIp + " " + std::to_string(_serverPort));
        }
        _networkInitDone = true;
    }

    if (!_scriptsLoaded) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "[Platformer] Loading game script..." << std::endl;
        sendMessage("LoadScript", "assets/scripts/platformer/Main.lua");
        _scriptsLoaded = true;
    }
}

} // namespace platformerGame
