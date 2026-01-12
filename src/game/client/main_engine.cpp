/**
 * @file main_engine.cpp
 * @brief Simplified R-Type client using GameEngine (Data-Driven Architecture)
 *
 * This is the NEW client entry point demonstrating the refactored architecture.
 * Compare with client/main.cpp (legacy version) to see the simplification.
 *
 * Usage:
 *   ./r-type_client_engine                          # Default (client.json)
 *   ./r-type_client_engine --config server.json      # Server mode
 *   ./r-type_client_engine --config local.json       # Solo mode
 *
 * All game logic, module loading, and script initialization is handled
 * by the GameEngine class via JSON configuration.
 */

#include "core/GameEngine.hpp"
#include <iostream>
#include <cstdlib>
#include <exception>

namespace {
    /**
     * @brief Parse command line arguments
     */
    std::string parseConfigPath(int argc, char** argv) {
        // Default configuration
        std::string configPath = "assets/config/client.json";

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "--config" || arg == "-c") {
                if (i + 1 < argc) {
                    configPath = argv[i + 1];
                    i++; // Skip next argument
                } else {
                    std::cerr << "ERROR: --config requires a path argument" << std::endl;
                    std::exit(1);
                }
            } else if (arg == "--server" || arg == "-s") {
                configPath = "assets/config/server.json";
            } else if (arg == "--local" || arg == "-l") {
                configPath = "assets/config/local.json";
            } else if (arg == "--help" || arg == "-h") {
                std::cout << "R-Type Engine - Usage:\n"
                          << "  --config <path>  Specify config file (default: client.json)\n"
                          << "  --server, -s     Use server.json\n"
                          << "  --local, -l      Use local.json\n"
                          << "  --help, -h       Show this message\n"
                          << "\nEnvironment Variables:\n"
                          << "  RTYPE_DEBUG=1    Enable debug logging\n";
                std::exit(0);
            } else {
                std::cerr << "WARNING: Unknown argument: " << arg << std::endl;
            }
        }

        return configPath;
    }

    /**
     * @brief Display startup banner
     */
    void printBanner() {
        if (std::getenv("RTYPE_DEBUG")) {
            std::cout << R"(
╔═══════════════════════════════════════════════════════════════╗
║              R-TYPE GAME ENGINE v2.0                          ║
║          Data-Driven Architecture (C++ + Lua)                 ║
╚═══════════════════════════════════════════════════════════════╝
)" << std::endl;
        }
    }
}

int main(int argc, char** argv) {
    printBanner();

    try {
        // Phase 1: Parse configuration path from command line
        std::string configPath = parseConfigPath(argc, argv);

        std::cout << "[Main] Starting R-Type Engine..." << std::endl;
        std::cout << "[Main] Configuration: " << configPath << std::endl;

        // Phase 2: Create and initialize engine
        rtypeEngine::GameEngine engine(configPath);

        // Phase 3: Run the game loop
        // This call blocks until engine.exit() is called or window is closed
        engine.run();

        // Phase 4: Cleanup (RAII - automatic)
        std::cout << "[Main] Engine stopped gracefully" << std::endl;

        return 0;

    } catch (const std::runtime_error& e) {
        // Configuration errors, module loading failures, etc.
        std::cerr << "[Main] FATAL ERROR: " << e.what() << std::endl;
        return 1;

    } catch (const std::exception& e) {
        // Unexpected errors
        std::cerr << "[Main] UNEXPECTED ERROR: " << e.what() << std::endl;
        return 2;

    } catch (...) {
        // Catch-all (should never happen with proper RAII)
        std::cerr << "[Main] UNKNOWN ERROR" << std::endl;
        return 3;
    }
}
