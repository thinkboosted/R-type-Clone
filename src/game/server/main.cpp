#include "RTypeServer.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {
int parseLagFlag(int argc, char **argv) {
  for (int i = 2; i < argc; ++i) {
    const std::string arg = argv[i];
    const std::string prefix = "--simulate-lag=";
    if (arg.rfind(prefix, 0) == 0) {
      return std::max(0, std::stoi(arg.substr(prefix.size())));
    }
  }
  return 0;
}

bool parsePrintBandwidthFlag(int argc, char **argv) {
  for (int i = 2; i < argc; ++i) {
    if (std::string(argv[i]) == "--print-bandwidth") {
      return true;
    }
  }
  return false;
}
}

int main(int argc, char **argv) {
  try {
    if (argc < 2) {
      std::cerr << "Usage: ./r-type_server <port> [--simulate-lag=100] [--print-bandwidth]" << std::endl;
      return 1;
    }

    int port = 0;
    try {
        port = std::stoi(argv[1]);
    } catch (...) {
        std::cerr << "Error: Invalid port number." << std::endl;
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

    rtypeGame::RTypeServer app(port, simulateLagMs, printBandwidth);

    app.loadModule("LuaECSManager");
    app.loadModule("BulletPhysicEngine");
    app.loadModule("ECSSavesManager");
    app.loadModule("NetworkManager");

    std::cout << "Starting Rtype Server on port " << port << "..." << std::endl;
    app.run();
    std::cout << "Rtype Server closed." << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}