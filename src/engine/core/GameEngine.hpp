#pragma once

#include "../app/AApplication.hpp"
#include "../modulesManager/IModulesManager.hpp"
#include <sol/sol.hpp>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace rtypeEngine {

/**
 * @brief Core Game Engine - Data-Driven Architecture
 *
 * Responsibilities:
 * - Fixed timestep game loop (accumulator pattern)
 * - Module lifecycle management (load order, pipeline execution)
 * - Configuration-driven initialization (JSON)
 * - Lua scripting integration (Sol2)
 *
 * Design Pattern:
 * - Inversion of Control: Client code only calls run()
 * - Pipeline Architecture: Modules execute in defined phases
 * - Data-Driven: All configuration externalized to JSON
 *
 * Memory Safety:
 * - RAII: All resources cleaned up in destructor
 * - No raw pointers exposed
 * - Modules owned via shared_ptr (inherited from AApplication)
 */
class GameEngine : public AApplication {
public:
    /**
     * @brief Configuration structure (parsed from JSON)
     */
    struct EngineConfig {
        // Timing configuration
        double fixedTimestep = 1.0 / 60.0;  // Physics/Network update rate (16.67ms)
        int maxFPS = 144;                    // Render cap (0 = unlimited)
        double maxFrameTime = 0.25;          // Clamp excessive deltas (prevents spiral of death)

        // Module loading
        std::vector<std::string> modules;    // Ordered list of .so/.dll to load
        std::vector<std::string> moduleOrder; // Execution order for pipeline

        // Scripting
        std::vector<std::string> startupScripts; // Auto-loaded Lua scripts
        bool enableLuaDebug = false;

        // Networking
        std::string networkMode = "client";  // "client", "server", "local"
        std::string serverAddress = "127.0.0.1:5555";
    };

    /**
     * @brief Construct GameEngine from JSON config
     * @param configPath Path to engine configuration (e.g., "assets/config/client.json")
     * @throws std::runtime_error if config file is invalid or missing
     */
    explicit GameEngine(const std::string& configPath);

    /**
     * @brief Destructor - RAII cleanup
     */
    ~GameEngine() override;

    /**
     * @brief Initialize engine subsystems
     *
     * Execution order:
     * 1. Parse configuration
     * 2. Setup ZeroMQ broker
     * 3. Load modules (in order specified by config)
     * 4. Initialize Lua state + bindings
     * 5. Load startup scripts
     * 6. Subscribe to critical events
     */
    void init() override;

    /**
     * @brief Single frame update (called by run() loop)
     *
     * Implements fixed timestep accumulator:
     * - Measures frame delta time
     * - Accumulates time debt
     * - Executes fixed updates (60 Hz) until debt is paid
     * - Executes variable update (render) once per frame
     */
    void loop() override;

    /**
     * @brief Get current engine configuration
     */
    const EngineConfig& getConfig() const { return _config; }

    /**
     * @brief Load a Lua script (can be called from C++ or Lua)
     * @param scriptPath Relative path from workspace root
     * @throws std::runtime_error if script fails to load
     */
    void loadLuaScript(const std::string& scriptPath);

private:
    // Configuration
    EngineConfig _config;
    std::string _configPath;

    // Lua scripting
    sol::state _lua;
    std::vector<std::string> _loadedScripts; // Track loaded scripts (prevent duplicates)

    // Timing (Fixed Timestep Pattern)
    std::chrono::high_resolution_clock::time_point _lastFrameTime;
    double _accumulator = 0.0;
    uint64_t _frameCount = 0;

    // Frame rate limiting
    std::chrono::high_resolution_clock::time_point _lastRenderTime;
    double _minFrameTime = 0.0; // Calculated from maxFPS

    /**
     * @brief Parse JSON configuration file
     * @throws std::runtime_error on parse error
     */
    void loadConfiguration(const std::string& path);

    /**
     * @brief Initialize Lua state and expose engine APIs
     */
    void initializeLua();

    /**
     * @brief Setup Lua bindings for engine functions
     */
    void setupLuaBindings();

    /**
     * @brief Load modules in order specified by config
     */
    void loadModules();

    /**
     * @brief Execute module pipeline for one fixed timestep
     *
     * Pipeline phases (order matters!):
     * 1. INPUT    - Poll user input (keyboard, mouse, gamepad)
     * 2. NETWORK  - Receive network packets, sync state
     * 3. PHYSICS  - Bullet3 simulation step
     * 4. ECS      - Lua systems update (gameplay logic)
     * 5. RENDER   - Draw frame (interpolated with alpha)
     */
    void executeFixedUpdate();

    /**
     * @brief Execute render phase with interpolation
     * @param alpha Interpolation factor [0.0 - 1.0] between physics states
     */
    void executeRenderUpdate(double alpha);

    /**
     * @brief Publish frame timing metrics to modules
     */
    void publishFrameMetrics();

    /**
     * @brief Validate configuration (check for missing critical modules)
     * @throws std::runtime_error if configuration is invalid
     */
    void validateConfiguration();
};

} // namespace rtypeEngine
