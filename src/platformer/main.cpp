#include "PlatformerGame.hpp"
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    try {
        platformerGame::PlatformerGame app;

        app.loadModule("LuaECSManager");
        app.loadModule("GLEWSFMLRenderer");
        app.loadModule("SFMLWindowManager");
        app.loadModule("SFMLSoundManager");
        app.loadModule("BulletPhysicEngine");

        std::cout << "Starting 3D Platformer..." << std::endl;
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
