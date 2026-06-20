#include "RTypeClient.hpp"
#include <iostream>
#include <thread>
#include <chrono>

#ifdef _WIN32
    #include <process.h>
#else
    #include <unistd.h>
#endif

namespace rtypeGame {

namespace {
int processId() {
#ifdef _WIN32
    return _getpid();
#else
    return static_cast<int>(getpid());
#endif
}
} // namespace

RTypeClient::RTypeClient(bool isLocal, const std::string& serverIp, int serverPort,
                                                 int simulateLagMs, bool printBandwidth)
        : RTypeGame(), _isLocal(isLocal), _serverIp(serverIp), _serverPort(serverPort),
            _simulateLagMs(simulateLagMs), _printBandwidth(printBandwidth) {
#ifdef _WIN32
    setupBroker("127.0.0.1:*", true);
#else
    // Use an IPC namespace bound to PID so each local client process has an isolated bus.
    const std::string busNamespace = "ipc:///tmp/rtype-client-bus-" + std::to_string(processId());
    setupBroker(busNamespace, true);
#endif
}

void RTypeClient::onInit() {
    std::cout << "[Client] Initializing (" << (_isLocal ? "Local" : "Network") << ")..." << std::endl;
}

void RTypeClient::onLoop() {
    if (!_networkInitDone) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        if (_printBandwidth) {
            std::cout << "[Client] Bandwidth logging enabled" << std::endl;
            sendMessage("RequestNetworkSetBandwidthLogging", "true");
        }

        if (_simulateLagMs > 0) {
            std::cout << "[Client] Enabling simulated lag: " << _simulateLagMs << "ms" << std::endl;
            sendMessage("RequestNetworkSetLag", std::to_string(_simulateLagMs));
        }

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
