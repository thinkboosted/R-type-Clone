#include "Rtype.hpp"
#include "modulesManager/ModulesManager.hpp"
#include <iostream>

namespace rtypeGame {

RTypeGame::RTypeGame() : AApplication() {
    this->_modulesManager = std::make_shared<rtypeGame::ModulesManager>();
}

void RTypeGame::loadModule(const std::string &moduleName) {
    std::string path = moduleName;
#ifdef _WIN32
    path += ".dll";
#else
    path += ".so";
#endif
    this->addModule(path, this->_pubBrokerEndpoint, this->_subBrokerEndpoint);
}

void RTypeGame::init() {
    subscribe("NetworkStatus", [](const std::string &payload) {
        std::cout << "[NetworkStatus] " << payload << std::endl;
    });

    subscribe("NetworkError", [](const std::string &payload) {
        std::cerr << "[NetworkError] " << payload << std::endl;
    });

    onInit();
}

void RTypeGame::loop() {
    onLoop();
}

} // namespace rtypeGame
