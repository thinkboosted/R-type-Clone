#include "RTypeClient.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {
int parseLagFlag(int argc, char **argv) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const std::string prefix = "--simulate-lag=";
    if (arg.rfind(prefix, 0) == 0) {
      return std::max(0, std::stoi(arg.substr(prefix.size())));
    }
  }
  return 0;
}

bool parsePrintBandwidthFlag(int argc, char **argv) {
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--print-bandwidth") {
      return true;
    }
  }
  return false;
}
}

int main(int argc, char **argv) {
  try {
    bool isLocal = false;
    std::string serverIp = "";
    int serverPort = 0;

    if (argc >= 2 && std::string(argv[1]) == "local") {
        isLocal = true;
    } else if (argc >= 3) {
        serverIp = argv[1];
        try {
            serverPort = std::stoi(argv[2]);
        } catch (...) {
            std::cerr << "Error: Invalid port number." << std::endl;
            return 1;
        }
    } else {
      std::cerr << "Usage: ./r-type_client local [--simulate-lag=100] [--print-bandwidth]  OR  ./r-type_client <server_ip> <server_port> [--simulate-lag=100] [--print-bandwidth]" << std::endl;
        return 1;
    }

    int simulateLagMs = 0;
    bool printBandwidth = false;
    try {
      simulateLagMs = parseLagFlag(argc, argv);
      printBandwidth = parsePrintBandwidthFlag(argc, argv);
    } catch (...) {
      std::cerr << "Error: Invalid simulate-lag value." << std::endl;
      return 1;
    }

    rtypeGame::RTypeClient app(isLocal, serverIp, serverPort, simulateLagMs,
                               printBandwidth);

    app.loadModule("LuaECSManager");
    app.loadModule("GLEWSFMLRenderer");
    app.loadModule("SFMLWindowManager");
    app.loadModule("SFMLSoundManager");
    app.loadModule("BulletPhysicEngine");
    app.loadModule("ECSSavesManager");
    app.loadModule("NetworkManager");

    std::cout << "Starting Rtype Client..." << std::endl;
    app.run();
    std::cout << "Rtype Client closed." << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "CRITICAL ERROR CAUGHT IN MAIN: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "UNKNOWN CRITICAL ERROR CAUGHT IN MAIN" << std::endl;
    return 1;
  }
  return 0;
}