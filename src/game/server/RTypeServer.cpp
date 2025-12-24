#include "RTypeServer.hpp"
#include <iostream>
#include <thread>
#include <chrono>

namespace rtypeGame {

RTypeServer::RTypeServer(int port) : RTypeGame(), _port(port) {

    std::string endpoint = "127.0.0.1:" + std::to_string(port);
    setupBroker(endpoint, true);
}

void RTypeServer::onInit() {
    std::cout << "[Server] Initializing..." << std::endl;
}

void RTypeServer::onLoop() {
    if (!_networkInitDone) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "[Server] Requesting Bind on port " << _port << std::endl;
        sendMessage("RequestNetworkBind", std::to_string(_port));
        _networkInitDone = true;
    }

    if (!_scriptsLoaded) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "[Server] Loading game logic..." << std::endl;
        sendMessage("LoadScript", "assets/scripts/space-shooter/GameLoop.lua");
        _scriptsLoaded = true;
    }
}

} // namespace rtypeGame
