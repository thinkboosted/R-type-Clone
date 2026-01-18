#include "RTypeClient.hpp"
#include <iostream>
#include <string>
#include <vector>

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
        std::cerr << "Usage: ./r-type_client local  OR  ./r-type_client <server_ip> <server_port>" << std::endl;
        return 1;
    }

    rtypeGame::RTypeClient app(isLocal, serverIp, serverPort);

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