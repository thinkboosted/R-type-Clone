#include "LuaECS.hpp"
#include <iostream>
#include <sstream>
#include <random>

namespace rtypeEngine {

    LuaECS::LuaECS(const char* pubEndpoint, const char* subEndpoint)
        : AModule(pubEndpoint, subEndpoint) {
    }

    LuaECS::~LuaECS() {
    }

    void LuaECS::init() {
        _lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::table, sol::lib::math);
        setupLuaBindings();

        subscribe("LoadSystemScript", [this](const std::string& msg) {
            this->loadSystemScript(msg);
        });

        subscribe("LoadComponentScript", [this](const std::string& msg) {
            this->loadComponentScript(msg);
        });

        std::cout << "[LuaECS] Initialized" << std::endl;
    }

    std::string generateUuid() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        static std::uniform_int_distribution<> dis2(8, 11);

        std::stringstream ss;
        int i;
        ss << std::hex;
        for (i = 0; i < 8; i++) {
            ss << dis(gen);
        }
        ss << "-";
        for (i = 0; i < 4; i++) {
            ss << dis(gen);
        }
        ss << "-4";
        for (i = 0; i < 3; i++) {
            ss << dis(gen);
        }
        ss << "-";
        ss << dis2(gen);
        for (i = 0; i < 3; i++) {
            ss << dis(gen);
        }
        ss << "-";
        for (i = 0; i < 12; i++) {
            ss << dis(gen);
        }
        return ss.str();
    }

    void LuaECS::setupLuaBindings() {
        auto scene = _lua.create_named_table("Scene");

        scene.set_function("createEntity", [this](const std::string& type) -> std::string {
            std::string id = generateUuid();
            std::stringstream ss;
            ss << "CreateEntity:" << type << ":" << id;
            sendMessage("SceneCommand", ss.str());
            std::cout << "[LuaECS] Requesting CreateEntity: " << type << " with ID: " << id << std::endl;
            return id;
        });

        scene.set_function("moveEntity", [this](const std::string& id, float x, float y, float z) {
            std::stringstream ss;
            ss << "MoveEntity:" << id << "," << x << "," << y << "," << z;
            sendMessage("SceneCommand", ss.str());
        });

        scene.set_function("rotateEntity", [this](const std::string& id, float x, float y, float z) {
            std::stringstream ss;
            ss << "RotateEntity:" << id << "," << x << "," << y << "," << z;
            sendMessage("SceneCommand", ss.str());
        });

        scene.set_function("setPosition", [this](const std::string& id, float x, float y, float z) {
            std::stringstream ss;
            ss << "SetPosition:" << id << "," << x << "," << y << "," << z;
            sendMessage("SceneCommand", ss.str());
        });

        scene.set_function("setLightProperties", [this](const std::string& id, float r, float g, float b, float intensity) {
            std::stringstream ss;
            ss << "SetLightProperties:" << id << "," << r << "," << g << "," << b << "," << intensity;
            sendMessage("SceneCommand", ss.str());
        });

        scene.set_function("setActiveCamera", [this](const std::string& id) {
            std::stringstream ss;
            ss << "SetActiveCamera:" << id;
            sendMessage("SceneCommand", ss.str());
        });
    }

    void LuaECS::loadComponentScript(const std::string& path) {
        try {
            _lua.script_file(path);
            std::cout << "[LuaECS] Loaded component script: " << path << std::endl;
        } catch (const sol::error& e) {
            std::cerr << "[LuaECS] Error loading component script: " << e.what() << std::endl;
        }
    }

    void LuaECS::loadSystemScript(const std::string& path) {
        try {
            sol::table system = _lua.script_file(path);
            if (system.valid()) {
                _systems.push_back(system);
                if (system["init"].valid()) {
                    system["init"]();
                }
                std::cout << "[LuaECS] Loaded system script: " << path << std::endl;
            }
        } catch (const sol::error& e) {
            std::cerr << "[LuaECS] Error loading system script: " << e.what() << std::endl;
        }
    }

    void LuaECS::loop() {
        for (auto& system : _systems) {
            if (system["update"].valid()) {
                try {
                    system["update"](0.016f); // Mock dt
                } catch (const sol::error& e) {
                    std::cerr << "[LuaECS] Error in system update: " << e.what() << std::endl;
                }
            }
        }
    }

    void LuaECS::cleanup() {
        _systems.clear();
    }

}

extern "C" __declspec(dllexport) rtypeEngine::IModule* createModule(const char* pubEndpoint, const char* subEndpoint) {
    return new rtypeEngine::LuaECS(pubEndpoint, subEndpoint);
}
