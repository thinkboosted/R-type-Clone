#include "GameEngine.hpp"
#include "../modules/WindowManager/IWindowManager.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <mutex>
#include <thread>

using json = nlohmann::json;

namespace {
    // Helper: Check if debug mode is enabled
    bool isDebugEnabled() {
        static bool enabled = (std::getenv("RTYPE_DEBUG") != nullptr);
        return enabled;
    }

    // Helper: Format elapsed time
    std::string formatTime(double seconds) {
        std::ostringstream oss;
        oss.precision(2);
        oss << std::fixed << (seconds * 1000.0) << "ms";
        return oss.str();
    }
}

namespace rtypeEngine {

GameEngine::GameEngine(const std::string& configPath)
    : AApplication(), _configPath(configPath) {

    _lastFrameTime = std::chrono::high_resolution_clock::now();
    _lastRenderTime = _lastFrameTime;

    if (isDebugEnabled()) {
        std::cout << "[GameEngine] Constructing with config: " << configPath << std::endl;
    }
}

GameEngine::~GameEngine() {
    if (isDebugEnabled()) {
        std::cout << "[GameEngine] Destroying (frames rendered: " << _frameCount << ")" << std::endl;
    }

    _networkModule.reset();
    _windowModule.reset();
    _renderModule.reset();
    _ecsModule.reset();
    _physicsModule.reset();

    if (_modulesManager) {
        _modulesManager.reset();
    }

    {
        std::unique_lock<std::shared_mutex> lock(_moduleMutex);
        _modules.clear();
    }
}

void GameEngine::init() {
    if (isDebugEnabled()) {
        std::cout << "[GameEngine] ======== INITIALIZATION START ========" << std::endl;
    }

    // Phase 1: Load and validate configuration
    try {
        loadConfiguration(_configPath);
        validateConfiguration();
    } catch (const std::exception& e) {
        std::cerr << "[GameEngine] FATAL: Configuration error: " << e.what() << std::endl;
        throw;
    }

    // Phase 2: Calculate frame timing constraints
    if (_config.maxFPS > 0) {
        _minFrameTime = 1.0 / static_cast<double>(_config.maxFPS);
        if (isDebugEnabled()) {
            std::cout << "[GameEngine] Frame cap: " << _config.maxFPS
                      << " FPS (" << formatTime(_minFrameTime) << "/frame)" << std::endl;
        }
    }

    // Phase 3: Setup message broker (ZeroMQ)
    try {
        std::string brokerEndpoint;
        bool isServer = (_config.networkMode == "server");

        // Determine broker mode based on configuration
        if (_config.networkMode == "local") {
            brokerEndpoint = "local";  // Triggers inproc:// mode
        } else if (_config.networkMode == "server") {
            brokerEndpoint = "0.0.0.0:5555";  // Server binds to network
        } else {
            brokerEndpoint = "127.0.0.1:*";  // Client uses ephemeral ports
        }

        setupBroker(brokerEndpoint, isServer);

        if (isDebugEnabled()) {
            std::cout << "[GameEngine] Message broker active (mode: "
                      << _config.networkMode << ")" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[GameEngine] FATAL: Failed to initialize message broker: "
                  << e.what() << std::endl;
        throw;
    }

    // Phase 4: Initialize Lua scripting engine
    try {
        initializeLua();
    } catch (const std::exception& e) {
        std::cerr << "[GameEngine] FATAL: Lua initialization failed: " << e.what() << std::endl;
        throw;
    }

    // Phase 5: Load modules in specified order
    try {
        loadModules();
    } catch (const std::exception& e) {
        std::cerr << "[GameEngine] FATAL: Module loading failed: " << e.what() << std::endl;
        throw;
    }

    // Phase 7: Subscribe to critical engine events
    subscribe("ExitApplication", [this](const std::string&) {
        if (isDebugEnabled()) {
            std::cout << "[GameEngine] Exit requested" << std::endl;
        }
        _running = false;
    });

    subscribe("ReloadConfig", [this](const std::string&) {
        if (isDebugEnabled()) {
            std::cout << "[GameEngine] Config reload requested (not yet implemented)" << std::endl;
        }
        // TODO: Hot-reload configuration
    });

    if (isDebugEnabled()) {
        std::cout << "[GameEngine] ======== INITIALIZATION COMPLETE ========" << std::endl;
        std::cout << "[GameEngine] Loaded " << _modules.size() << " modules, "
                  << _loadedScripts.size() << " scripts" << std::endl;
    }
}

void GameEngine::run() {
    _running = true;

    // Ensure we can exit cleanly on ExitApplication
    subscribe("ExitApplication", [this](const std::string&) {
        this->_running = false;
    });

    // Standard initialization (config, broker, Lua state, load modules, subscribe events)
    init();

    if (isDebugEnabled()) {
        std::cout << "[App] Starting " << _modules.size() << " modules" << std::endl;
    }

    // Start all modules (their own threads will call init() and begin processing messages)
    for (const auto &module : _modules) {
        module->start();
    }

    // After modules are running, request LuaECSManager to load startup scripts into its own Lua state
    for (const auto& scriptPath : _config.startupScripts) {
        // Dispatch through message bus so LuaECSManager handles loading (it owns the ECS global)
        sendMessage("LoadScript", scriptPath);
        if (isDebugEnabled()) {
            std::cout << "=== Loading Unified GameLoop ===" << std::endl;
        }
    }

    bool windowReady = false;
    while (_running) {
        processMessages();

        if (_windowModule) {
            bool isOpen = static_cast<IWindowManager*>(_windowModule.get())->isOpen();
            if (!windowReady && isOpen) {
                windowReady = true;
                if (isDebugEnabled()) {
                    std::cout << "[GameEngine] Window is ready" << std::endl;
                }
            }
            if (windowReady && !isOpen) {
                std::cout << "[GameEngine] Window closed detected - initiating shutdown" << std::endl;
                _running = false;
                break;
            }
        }

        loop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "[GameEngine] Main loop exited - stopping all modules..." << std::endl;

    auto startShutdown = std::chrono::steady_clock::now();
    for (size_t i = 0; i < _modules.size(); ++i) {
        if (isDebugEnabled()) {
            std::cout << "[GameEngine] Stopping module " << (i+1) << "/" << _modules.size() << "..." << std::endl;
        }
        _modules[i]->stop();
    }

    auto shutdownDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startShutdown);

    std::cout << "[GameEngine] All modules stopped in " << shutdownDuration.count()
              << "ms - shutdown complete" << std::endl;

    cleanupMessageBroker();
}

void GameEngine::loop() {
    // Measure frame time
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> frameTimeDuration = currentTime - _lastFrameTime;
    double frameTime = frameTimeDuration.count();
    _lastFrameTime = currentTime;

    // Clamp excessive frame times (prevents "spiral of death" after debugger pause)
    if (frameTime > _config.maxFrameTime) {
        if (isDebugEnabled()) {
            std::cout << "[GameEngine] WARNING: Frame time clamped from "
                      << formatTime(frameTime) << " to "
                      << formatTime(_config.maxFrameTime) << std::endl;
        }
        frameTime = _config.maxFrameTime;
    }

    // Accumulate time debt
    _accumulator += frameTime;

    // Fixed timestep loop (deterministic physics/network updates)
    int fixedUpdateCount = 0;
    const int MAX_FIXED_UPDATES = 5; // Safety limit (prevents lockup)

    while (_accumulator >= _config.fixedTimestep) {
        if (fixedUpdateCount >= MAX_FIXED_UPDATES) {
            if (isDebugEnabled()) {
                std::cout << "[GameEngine] WARNING: Fixed update limit reached (slow frame)"
                          << std::endl;
            }
            _accumulator = 0.0; // Reset to prevent spiral
            break;
        }

        executeFixedUpdate();
        _accumulator -= _config.fixedTimestep;
        fixedUpdateCount++;
    }

    // Calculate interpolation factor for smooth rendering
    double alpha = _accumulator / _config.fixedTimestep;

    // Variable update (rendering with interpolation)
    executeRenderUpdate(alpha);

    // Frame rate limiting (if configured)
    if (_config.maxFPS > 0) {
        auto renderTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsedSinceRender = renderTime - _lastRenderTime;

        double timeToWait = _minFrameTime - elapsedSinceRender.count();
        if (timeToWait > 0.0) {
            auto waitDuration = std::chrono::duration<double>(timeToWait);
            std::this_thread::sleep_for(waitDuration);
        }

        _lastRenderTime = std::chrono::high_resolution_clock::now();
    }

    // Publish frame metrics (for profiling/debugging)
    if (_frameCount % 60 == 0 && isDebugEnabled()) {
        publishFrameMetrics();
    }

    _frameCount++;

    // Process ZeroMQ messages (dequeue pending messages)
    processMessages();
}

void GameEngine::loadConfiguration(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open configuration file: " + path);
    }

    json config;
    try {
        file >> config;
    } catch (const json::parse_error& e) {
        throw std::runtime_error("JSON parse error in " + path + ": " + e.what());
    }

    // Parse engine settings
    if (config.contains("engine")) {
        auto& engineCfg = config["engine"];

        if (engineCfg.contains("max_fps")) {
            _config.maxFPS = engineCfg["max_fps"].get<int>();
        }

        if (engineCfg.contains("fixed_step")) {
            _config.fixedTimestep = engineCfg["fixed_step"].get<double>();
        }

        if (engineCfg.contains("max_frame_time")) {
            _config.maxFrameTime = engineCfg["max_frame_time"].get<double>();
        }
    }

    // Parse module list
    if (config.contains("modules")) {
        for (const auto& moduleName : config["modules"]) {
            _config.modules.push_back(moduleName.get<std::string>());
        }
    }

    // Parse module execution order (optional, defaults to load order)
    if (config.contains("module_order")) {
        for (const auto& moduleName : config["module_order"]) {
            _config.moduleOrder.push_back(moduleName.get<std::string>());
        }
    } else {
        _config.moduleOrder = _config.modules; // Default: same as load order
    }

    // Parse startup scripts
    if (config.contains("startup_scripts")) {
        for (const auto& scriptPath : config["startup_scripts"]) {
            _config.startupScripts.push_back(scriptPath.get<std::string>());
        }
    } else if (config.contains("startup_script")) {
        // Legacy single script support
        _config.startupScripts.push_back(config["startup_script"].get<std::string>());
    }

    // Parse network settings
    if (config.contains("network")) {
        auto& netCfg = config["network"];

        if (netCfg.contains("mode")) {
            _config.networkMode = netCfg["mode"].get<std::string>();
        }

        if (netCfg.contains("server_address")) {
            _config.serverAddress = netCfg["server_address"].get<std::string>();
        }
    }

    // Parse Lua settings
    if (config.contains("lua")) {
        auto& luaCfg = config["lua"];

        if (luaCfg.contains("debug")) {
            _config.enableLuaDebug = luaCfg["debug"].get<bool>();
        }
    }

    if (isDebugEnabled()) {
        std::cout << "[GameEngine] Configuration loaded:" << std::endl;
        std::cout << "  - Fixed timestep: " << formatTime(_config.fixedTimestep) << std::endl;
        std::cout << "  - Max FPS: " << (_config.maxFPS > 0 ? std::to_string(_config.maxFPS) : "unlimited") << std::endl;
        std::cout << "  - Modules: " << _config.modules.size() << std::endl;
        std::cout << "  - Scripts: " << _config.startupScripts.size() << std::endl;
        std::cout << "  - Network mode: " << _config.networkMode << std::endl;
    }
}

void GameEngine::validateConfiguration() {
    // Check if at least one module is specified (allow empty for local/test mode)
    if (_config.modules.empty() && _config.networkMode != "local") {
        std::cerr << "[GameEngine] WARNING: No modules specified (only valid for local mode testing)" << std::endl;
    }

    // Validate fixed timestep range
    if (_config.fixedTimestep <= 0.0 || _config.fixedTimestep > 1.0) {
        throw std::runtime_error("Configuration error: fixed_step must be in range (0.0, 1.0]");
    }

    // Validate network mode
    if (_config.networkMode != "client" &&
        _config.networkMode != "server" &&
        _config.networkMode != "local") {
        throw std::runtime_error("Configuration error: network.mode must be 'client', 'server', or 'local'");
    }

    if (isDebugEnabled()) {
        std::cout << "[GameEngine] Configuration validated successfully" << std::endl;
    }
}

void GameEngine::initializeLua() {
    // Open standard Lua libraries
    _lua.open_libraries(
        sol::lib::base,
        sol::lib::package,
        sol::lib::string,
        sol::lib::table,
        sol::lib::math,
        sol::lib::io,
        sol::lib::os
    );

    // Setup Lua bindings for engine functions
    setupLuaBindings();

    if (isDebugEnabled()) {
        std::cout << "[GameEngine] Lua state initialized (Sol2)" << std::endl;
    }
}

void GameEngine::setupLuaBindings() {
    // Create Engine namespace in Lua
    auto engineTable = _lua.create_table("Engine");

    // Expose loadScript function
    engineTable.set_function("loadScript", [this](const std::string& path) {
        loadLuaScript(path);
    });

    // Expose sendMessage function (wrapper around ZeroMQ)
    engineTable.set_function("sendMessage", [this](const std::string& topic, const std::string& message) {
        sendMessage(topic, message);
    });

    // Expose configuration access (read-only)
    auto configTable = _lua.create_table("Config");
    configTable["fixedTimestep"] = _config.fixedTimestep;
    configTable["maxFPS"] = _config.maxFPS;
    configTable["networkMode"] = _config.networkMode;
    engineTable["config"] = configTable;

    // Expose frame counter (useful for Lua-side timing)
    engineTable.set_function("getFrameCount", [this]() {
        return _frameCount;
    });

    // Expose exit function
    engineTable.set_function("exit", [this]() {
        sendMessage("ExitApplication", "");
    });

    if (_config.enableLuaDebug) {
        std::cout << "[GameEngine] Lua bindings registered (debug mode enabled)" << std::endl;
    }
}

void GameEngine::loadModules() {
    if (_modules.empty() && !_config.modules.empty()) {
        // Acquire exclusive lock (writing to module list)
        std::unique_lock<std::shared_mutex> lock(_moduleMutex);

        for (const auto& moduleName : _config.modules) {
            std::string modulePath;

            // If moduleName already contains a path (/ or \ or ends with .so/.dll), use it as-is
            // Otherwise prepend lib/
            if (moduleName.find('/') != std::string::npos ||
                moduleName.find('\\') != std::string::npos ||
                moduleName.find(".so") != std::string::npos ||
                moduleName.find(".dll") != std::string::npos) {
                // Path already fully specified
                modulePath = moduleName;
            } else {
                // Legacy: just module name, prepend lib/
                modulePath = "lib/" + moduleName;

                // Add platform-specific extension
                #ifdef _WIN32
                    modulePath += ".dll";
                #else
                    modulePath += ".so";
                #endif
            }

            try {
                if (isDebugEnabled()) {
                    std::cout << "[GameEngine] Loading module: " << modulePath << std::endl;
                }

                // ═══════════════════════════════════════════════════════════════════════════
                // CRITICAL: Pass shared ZMQ context to prevent segfault
                // ═══════════════════════════════════════════════════════════════════════════
                // For inproc://, all sockets MUST share the same zmq::context_t
                // Without this, modules create their own context → segfault
                // ═══════════════════════════════════════════════════════════════════════════
                addModule(modulePath, _pubBrokerEndpoint, _subBrokerEndpoint, &_sharedZmqContext);

                // ═══════════════════════════════════════════════════════════════════════════
                // CACHE MODULE POINTERS (Hard-Wired Architecture)
                // ═══════════════════════════════════════════════════════════════════════════
                // Store pointers to critical modules for direct virtual calls
                // Detect by module path or name (order-independent)
                // ═══════════════════════════════════════════════════════════════════════════
                if (modulePath.find("BulletPhysicEngine") != std::string::npos) {
                    _physicsModule = _modules.back();
                } else if (modulePath.find("LuaECSManager") != std::string::npos) {
                    _ecsModule = _modules.back();
                } else if (modulePath.find("GLEWSFMLRenderer") != std::string::npos) {
                    _renderModule = _modules.back();
                } else if (modulePath.find("SFMLWindowManager") != std::string::npos) {
                    _windowModule = _modules.back();
                } else if (modulePath.find("NetworkManager") != std::string::npos) {
                    _networkModule = _modules.back();
                }
            } catch (const std::exception& e) {
                std::cerr << "[GameEngine] ERROR: Failed to load module '"
                          << moduleName << "': " << e.what() << std::endl;
                throw;
            }
        }
    }
}

void GameEngine::loadLuaScript(const std::string& scriptPath) {
    // Check if already loaded (prevent duplicate loading)
    if (std::find(_loadedScripts.begin(), _loadedScripts.end(), scriptPath) != _loadedScripts.end()) {
        if (isDebugEnabled()) {
            std::cout << "[GameEngine] Script already loaded: " << scriptPath << std::endl;
        }
        return;
    }

    try {
        _lua.script_file(scriptPath);
        _loadedScripts.push_back(scriptPath);

        if (isDebugEnabled()) {
            std::cout << "[GameEngine] Loaded Lua script: " << scriptPath << std::endl;
        }
    } catch (const sol::error& e) {
        std::string errorMsg = "Lua script error in '" + scriptPath + "': " + e.what();
        throw std::runtime_error(errorMsg);
    }
}

void GameEngine::executeFixedUpdate() {
    // ═══════════════════════════════════════════════════════════════
    // HARD-WIRED FIXED TIMESTEP PIPELINE (Deterministic, 60 Hz)
    // ═══════════════════════════════════════════════════════════════
    // Direct virtual calls replace ZeroMQ messages for performance
    // Order is CRITICAL for correct dependency flow!
    //
    // INPUT → NETWORK (recv) → PHYSICS → ECS → NETWORK (send)
    //
    // Performance: ~10-50x faster than message passing
    // ═══════════════════════════════════════════════════════════════

    const double fixedDt = _config.fixedTimestep;

    // Phase 1: Input processing (window events)
    if (_windowModule) {
        _windowModule->fixedUpdate(fixedDt);
    }

    // Phase 2: Network receive (download state updates from server)
    if (_networkModule) {
        _networkModule->fixedUpdate(fixedDt);
    }

    // Phase 3: Physics simulation (Bullet3 stepSimulation)
    if (_physicsModule) {
        _physicsModule->fixedUpdate(fixedDt);
    }

    // Phase 4: ECS/Lua gameplay logic (collision handlers, AI, spawning)
    if (_ecsModule) {
        _ecsModule->fixedUpdate(fixedDt);
    }

    // Note: RENDER phase is in executeRenderUpdate (variable timestep)
}

void GameEngine::executeRenderUpdate(double alpha) {
    // ═══════════════════════════════════════════════════════════════
    // HARD-WIRED VARIABLE TIMESTEP RENDER (Smooth, interpolated)
    // ═══════════════════════════════════════════════════════════════
    // Alpha factor: [0.0 - 1.0] interpolation between physics states
    // This prevents stuttering when render FPS != physics FPS
    // Direct virtual calls for minimal latency
    // ═══════════════════════════════════════════════════════════════

    // Phase 1: Render module generates pixel buffer (OpenGL/GLEW)
    if (_renderModule) {
        _renderModule->render(alpha);
    }

    // Phase 2: Window module displays frame (SFML window.display())
    if (_windowModule) {
        _windowModule->render(alpha);
    }
}

void GameEngine::publishFrameMetrics() {
    std::ostringstream metrics;
    metrics << "Frame:" << _frameCount
            << " Accumulator:" << (_accumulator * 1000.0) << "ms";

    if (isDebugEnabled()) {
        std::cout << "[GameEngine] " << metrics.str() << std::endl;
    }

    sendMessage("FrameMetrics", metrics.str());
}

bool GameEngine::invokeModulePhase(const std::string& phase) {
    // ═══════════════════════════════════════════════════════════════
    // THREAD-SAFE MODULE INVOCATION
    // ═══════════════════════════════════════════════════════════════
    // Uses shared_lock for concurrent reads (modules don't modify
    // the module list during execution, only during init/cleanup)
    //
    // Fallback: If direct module access fails, use message bus
    // ═══════════════════════════════════════════════════════════════

    try {
        // Acquire shared lock (allows multiple readers)
        std::shared_lock<std::shared_mutex> lock(_moduleMutex);

        // Publish phase message (modules subscribe via ZeroMQ)
        // This is the PRIMARY mechanism (existing modules use this)
        sendMessage("PipelinePhase", phase);

        // Future enhancement: Direct module->update() calls
        // for (auto& module : _modules) {
        //     if (module->supportsPhase(phase)) {
        //         module->executePhase(phase);
        //     }
        // }

        return true;

    } catch (const std::exception& e) {
        std::cerr << "[GameEngine] ERROR in phase '" << phase << "': "
                  << e.what() << std::endl;
        return false;
    }
}

} // namespace rtypeEngine
