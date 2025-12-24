#include "RTypeClient.hpp"
#include <iostream>
#include <thread>
#include <chrono>

namespace rtypeGame {

RTypeClient::RTypeClient(bool isLocal, const std::string& serverIp, int serverPort)
    : RTypeGame(), _isLocal(isLocal), _serverIp(serverIp), _serverPort(serverPort) {

    setupBroker("127.0.0.1:*", true);
}

void RTypeClient::onInit() {
    std::cout << "[Client] Initializing (" << (_isLocal ? "Local" : "Network") << ")..." << std::endl;
}

void RTypeClient::onLoop() {
    if (!_networkInitDone) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        if (_isLocal) {
            std::cout << "[Client] Local solo mode: skipping network connection" << std::endl;
        } else {
            std::string payload = _serverIp + " " + std::to_string(_serverPort);
            std::cout << "[Client] Requesting Connect to " << payload << std::endl;
            sendMessage("RequestNetworkConnect", payload);

            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            sendMessage("RequestNetworkSend", "HELLO");
        }
        _networkInitDone = true;
    }

    if (!_scriptsLoaded) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "[Client] Loading game logic..." << std::endl;

        sendMessage("LoadScript", "assets/scripts/space-shooter/GameLoop.lua");

        _scriptsLoaded = true;
    }
}

} // namespace rtypeGame
