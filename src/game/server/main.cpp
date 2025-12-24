#include "RTypeServer.hpp"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char **argv) {
  try {
    if (argc < 2) {
      std::cerr << "Usage: ./r-type_server <port>" << std::endl;
      return 1;
    }

    int port = 0;
    try {
        port = std::stoi(argv[1]);
    } catch (...) {
        std::cerr << "Error: Invalid port number." << std::endl;
        return 1;
    }

    rtypeGame::RTypeServer app(port);

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