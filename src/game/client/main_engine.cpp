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
    void printHelp() {
        std::cout << "R-Type Engine - Usage:\n"
                  << "  ./r-type_client_engine [mode]\n"
                  << "\nModes (aliases):\n"
                  << "  local  | --local  | -l   -> assets/config/local.json\n"
                  << "  server | --server | -s   -> assets/config/server.json\n"
                  << "  client | --client | -c   -> assets/config/client.json (default)\n"
                  << "\nAdvanced:\n"
                  << "  --config <path>          -> custom config file\n"
                  << "  --help   | -h            -> show this help\n"
                  << "\nEnvironment Variables:\n"
                  << "  RTYPE_DEBUG=1            Enable debug logging\n";
    }

    struct ParsedArgs {
        std::string configPath;
        std::string modeLabel; // HUMAN readable mode (CLIENT/LOCAL/SERVER/custom)
        bool ok = true;
    };

    ParsedArgs parseConfigPath(int argc, char** argv) {
        ParsedArgs parsed;
        parsed.configPath = "assets/config/client.json";
        parsed.modeLabel = "CLIENT";

        auto setMode = [&](const std::string& mode, const std::string& path) {
            parsed.modeLabel = mode;
            parsed.configPath = path;
        };

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "--config" || arg == "-c") {
                if (i + 1 < argc) {
                    parsed.configPath = argv[++i];
                    parsed.modeLabel = "CUSTOM";
                } else {
                    std::cerr << "ERROR: --config requires a path argument" << std::endl;
                    parsed.ok = false;
                    return parsed;
                }
            } else if (arg == "--server" || arg == "-s" || arg == "server") {
                setMode("SERVER", "assets/config/server.json");
            } else if (arg == "--local" || arg == "-l" || arg == "local") {
                setMode("LOCAL", "assets/config/local.json");
            } else if (arg == "--client" || arg == "client") {
                setMode("CLIENT", "assets/config/client.json");
            } else if (arg == "--help" || arg == "-h" || arg == "help") {
                printHelp();
                parsed.ok = false; // Signal caller to exit gracefully
                return parsed;
            } else {
                std::cerr << "ERROR: Unknown argument: " << arg << "\n\n";
                printHelp();
                parsed.ok = false;
                return parsed;
            }
        }

        return parsed;
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
        ParsedArgs parsed = parseConfigPath(argc, argv);
        if (!parsed.ok) {
            return 1; // Help displayed or error encountered
        }

        std::cout << "[Main] Mode selected: " << parsed.modeLabel
                  << " (config: " << parsed.configPath << ")" << std::endl;
        std::cout << "[Main] Starting R-Type Engine..." << std::endl;
        std::cout << "[Main] Configuration: " << parsed.configPath << std::endl;

        // Phase 2: Create and initialize engine
        rtypeEngine::GameEngine engine(parsed.configPath);

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
