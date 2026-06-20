#include "PlatformerGame.hpp"
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    try {
        platformerGame::PlatformerMode mode = platformerGame::PlatformerMode::Solo;
        std::string serverIp = "127.0.0.1";
        int serverPort = 1234;

        if (argc >= 2) {
            std::string firstArg = argv[1];
            if (firstArg == "host") {
                mode = platformerGame::PlatformerMode::Host;
                if (argc >= 3) {
                    serverPort = std::stoi(argv[2]);
                }
            } else if (firstArg == "connect") {
                if (argc < 4) {
                    std::cerr << "Usage: ./platformer connect <host_ip> <port>" << std::endl;
                    std::cerr << "   or: ./platformer <host_ip> <port>" << std::endl;
                    return 1;
                }
                mode = platformerGame::PlatformerMode::Client;
                serverIp = argv[2];
                serverPort = std::stoi(argv[3]);
            } else if (argc >= 3) {
                // Backward-compatible shortcut: ./platformer <host_ip> <port>
                mode = platformerGame::PlatformerMode::Client;
                serverIp = argv[1];
                serverPort = std::stoi(argv[2]);
            }
        }

        platformerGame::PlatformerGame app(mode, serverIp, serverPort);

        app.loadModule("LuaECSManager");
        app.loadModule("BulletPhysicEngine");
        app.loadModule("NetworkManager");
        app.loadModule("GLEWSFMLRenderer");
        app.loadModule("SFMLWindowManager");
        app.loadModule("SFMLSoundManager");

        if (mode == platformerGame::PlatformerMode::Host) {
            std::cout << "Starting 3D Platformer Host on port " << serverPort << "..." << std::endl;
        } else if (mode == platformerGame::PlatformerMode::Client) {
            std::cout << "Starting 3D Platformer Client -> " << serverIp << ":" << serverPort << "..." << std::endl;
        } else {
            std::cout << "Starting 3D Platformer (Solo)..." << std::endl;
        }
        app.run();
        std::cout << "3D Platformer closed." << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "CRITICAL ERROR CAUGHT IN MAIN: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "UNKNOWN CRITICAL ERROR CAUGHT IN MAIN" << std::endl;
        return 1;
    }
    return 0;
}
