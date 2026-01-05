/**
 * @file main.cpp
 * @brief Test application for SFMLSoundManager module via LuaECSManager
 * 
 * This standalone test verifies that the sound system works correctly
 * when messages are sent through Lua (like the actual game does).
 * It loads both LuaECSManager and SFMLSoundManager modules.
 */

#include "../engine/app/AApplication.hpp"
#include "../engine/modulesManager/ModulesManager.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

/**
 * @brief Test application that loads LuaECSManager and SoundManager modules
 */
class TestSoundApp : public rtypeEngine::AApplication {
public:
    TestSoundApp() : AApplication(), _testComplete(false) {
        _modulesManager = std::make_shared<rtypeGame::ModulesManager>();
        // Setup broker in server mode (we are the message broker host)
        setupBroker("127.0.0.1:*", true);
    }
    
    ~TestSoundApp() override = default;
    
    bool isTestComplete() const { return _testComplete; }
    
    void markTestComplete() { _testComplete = true; }

    void loadModule(const std::string &moduleName) {
        std::string path = moduleName;
        #ifdef _WIN32
            path += ".dll";
        #else
            path += ".so";
        #endif
        addModule(path, _pubBrokerEndpoint, _subBrokerEndpoint);
    }

    void loadLuaScript(const std::string &scriptPath) {
        // Send message to LuaECSManager to load a script
        sendMessage("LoadScript", scriptPath);
    }
    
    void init() override {
        std::cout << "[TestSoundApp] Initialized" << std::endl;
    }
    
    void loop() override {
        // Nothing special in loop for this test
    }

private:
    std::atomic<bool> _testComplete;
};

int main(int argc, char** argv) {
    std::cout << "=== Sound System Test (via Lua ECS) ===" << std::endl;
    std::cout << "This test demonstrates the sound system with:" << std::endl;
    std::cout << "  - Laser and explosion sound effects" << std::endl;
    std::cout << "  - Background music (looping)" << std::endl;
    std::cout << "  - All message topics (Play, Stop, Volume, Pause/Resume)" << std::endl;
    std::cout << "  - All test logic runs in Lua via ECS messages" << std::endl;
    std::cout << std::endl;

    try {
        TestSoundApp app;
        
        std::cout << "[Test] Loading LuaECSManager module..." << std::endl;
        app.loadModule("LuaECSManager");
        
        std::cout << "[Test] Loading SFMLSoundManager module..." << std::endl;
        app.loadModule("SFMLSoundManager");
        
        // Run the app in a separate thread
        std::thread appThread([&app]() {
            app.run();
        });
        
        // Subscribe to TestComplete message from Lua
        app.subscribe("TestComplete", [&app](const std::string& msg) {
            std::cout << "[Test] Received TestComplete signal from Lua script" << std::endl;
            app.markTestComplete();
        });
        
        // Wait for modules to initialize
        std::cout << "[Test] Waiting for module initialization..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // Load the Lua test script - it will handle all testing
        std::cout << "[Test] Loading Lua test script (handles all sound tests)..." << std::endl;
        app.loadLuaScript("assets/scripts/test-sound/SoundTest.lua");
        
        // Wait for Lua script to signal completion
        std::cout << "[Test] Running Lua-driven tests (waiting for completion signal)..." << std::endl;
        std::cout << "[Test] Listen for laser, explosion, and music sounds!" << std::endl;
        
        while (!app.isTestComplete()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Exit the application
        std::cout << std::endl;
        std::cout << "[Test] Sending ExitApplication message..." << std::endl;
        app.sendMessage("ExitApplication", "");
        
        // Wait for the app thread to finish
        if (appThread.joinable()) {
            appThread.join();
        }
        
        std::cout << std::endl;
        std::cout << "=== Sound Test Complete ===" << std::endl;
        std::cout << "All tests run from Lua ECS script!" << std::endl;
        std::cout << "Check the output above for [SoundTest] and [SFMLSoundManager] messages." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
