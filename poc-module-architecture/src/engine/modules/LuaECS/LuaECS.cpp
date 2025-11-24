#include "LuaECS.hpp"
#include <iostream>
#include <sstream>
#include <random>
#include <cmath>
#include <thread>
#include <chrono>
#include <algorithm>

namespace rtypeEngine {

    LuaECS::LuaECS(const char* pubEndpoint, const char* subEndpoint)
        : AModule(pubEndpoint, subEndpoint) {
    }

    LuaECS::~LuaECS() {
    }

    void LuaECS::init() {
        _lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::table, sol::lib::math);
        setupLuaBindings();

        subscribe("LoadScript", [this](const std::string& msg) {
            this->loadScript(msg);
        });

        auto forwardEvent = [this](const std::string& eventName, const std::string& msg) {
             for (auto& system : _systems) {
                if (system[eventName].valid()) {
                    try {
                        system[eventName](msg);
                    } catch (const sol::error& e) {
                        std::cerr << "[LuaECS] Error in system " << eventName << ": " << e.what() << std::endl;
                    }
                }
            }
        };

        subscribe("KeyPressed", [=](const std::string& msg) { forwardEvent("onKeyPressed", msg); });
        subscribe("KeyReleased", [=](const std::string& msg) { forwardEvent("onKeyReleased", msg); });
        subscribe("MousePressed", [=](const std::string& msg) { forwardEvent("onMousePressed", msg); });
        subscribe("MouseReleased", [=](const std::string& msg) { forwardEvent("onMouseReleased", msg); });
        subscribe("MouseMoved", [=](const std::string& msg) { forwardEvent("onMouseMoved", msg); });

        subscribe("PhysicEvent", [this](const std::string& msg) {
            std::stringstream ss(msg);
            std::string segment;
            while (std::getline(ss, segment, ';')) {
                if (segment.empty()) continue;
                size_t split1 = segment.find(':');
                if (split1 == std::string::npos) continue;
                std::string command = segment.substr(0, split1);
                std::string data = segment.substr(split1 + 1);

                if (command == "Collision") {
                     for (auto& system : _systems) {
                        if (system["onCollision"].valid()) {
                            size_t split2 = data.find(':');
                            if (split2 != std::string::npos) {
                                std::string id1 = data.substr(0, split2);
                                std::string id2 = data.substr(split2 + 1);
                                try {
                                    system["onCollision"](id1, id2);
                                } catch (const sol::error& e) {
                                    std::cerr << "[LuaECS] Error in onCollision: " << e.what() << std::endl;
                                }
                            }
                        }
                    }
                } else if (command == "RaycastHit") {
                    size_t split2 = data.find(':');
                    if (split2 != std::string::npos) {
                        std::string id = data.substr(0, split2);
                        float distance = std::stof(data.substr(split2 + 1));
                        for (auto& system : _systems) {
                            if (system["onRaycastHit"].valid()) {
                                try {
                                    system["onRaycastHit"](id, distance);
                                } catch (const sol::error& e) {
                                    std::cerr << "[LuaECS] Error in onRaycastHit: " << e.what() << std::endl;
                                }
                            }
                        }
                    }
                }
            }
        });

        subscribe("EntityUpdated", [this](const std::string& msg) {
            std::stringstream ss(msg);
            std::string segment;
            while (std::getline(ss, segment, ';')) {
                if (segment.empty()) continue;
                // Format: EntityUpdated:id:x,y,z:rx,ry,rz
                size_t split1 = segment.find(':'); // After EntityUpdated
                if (split1 == std::string::npos) continue;

                std::string rest = segment.substr(split1 + 1);
                size_t split2 = rest.find(':'); // After id
                if (split2 == std::string::npos) continue;

                std::string id = rest.substr(0, split2);
                std::string coords = rest.substr(split2 + 1);

                size_t split3 = coords.find(':'); // After pos
                if (split3 == std::string::npos) continue;

                std::string posStr = coords.substr(0, split3);
                std::string rotStr = coords.substr(split3 + 1);

                float x, y, z, rx, ry, rz;
                char comma;
                std::stringstream pss(posStr);
                pss >> x >> comma >> y >> comma >> z;

                std::stringstream rss(rotStr);
                rss >> rx >> comma >> ry >> comma >> rz;

                for (auto& system : _systems) {
                    if (system["onEntityUpdated"].valid()) {
                        try {
                            system["onEntityUpdated"](id, x, y, z, rx, ry, rz);
                        } catch (const sol::error& e) {
                            std::cerr << "[LuaECS] Error in onEntityUpdated: " << e.what() << std::endl;
                        }
                    }
                }
            }
        });

        std::cout << "[LuaECS] Initialized" << std::endl;
    }

    std::string LuaECS::generateUuid() {
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
        auto ecs = _lua.create_named_table("ECS");

        ecs.set_function("createEntity", [this]() -> std::string {
            std::string id = generateUuid();
            _entities.push_back(id);
            _components[id] = std::map<std::string, sol::table>();
            return id;
        });

        ecs.set_function("destroyEntity", [this](const std::string& id) {
            auto it = std::find(_entities.begin(), _entities.end(), id);
            if (it != _entities.end()) {
                _entities.erase(it);
                _components.erase(id);
            }
        });

        ecs.set_function("addComponent", [this](const std::string& id, const std::string& name, sol::table data) {
            if (_components.find(id) != _components.end()) {
                _components[id][name] = data;
            }
        });

        ecs.set_function("removeComponent", [this](const std::string& id, const std::string& name) {
            if (_components.find(id) != _components.end()) {
                _components[id].erase(name);
            }
        });

        ecs.set_function("getComponent", [this](const std::string& id, const std::string& name) -> sol::object {
            if (_components.find(id) != _components.end()) {
                auto& comps = _components[id];
                if (comps.find(name) != comps.end()) {
                    return comps[name];
                }
            }
            return sol::nil;
        });

        ecs.set_function("getEntitiesWith", [this](sol::table components) -> std::vector<std::string> {
            std::vector<std::string> result;
            std::vector<std::string> required;
            for (auto& kv : components) {
                if (kv.second.is<std::string>()) {
                    required.push_back(kv.second.as<std::string>());
                }
            }

            for (const auto& entityId : _entities) {
                bool hasAll = true;
                const auto& entityComps = _components[entityId];
                for (const auto& req : required) {
                    if (entityComps.find(req) == entityComps.end()) {
                        hasAll = false;
                        break;
                    }
                }
                if (hasAll) {
                    result.push_back(entityId);
                }
            }
            return result;
        });

        ecs.set_function("sendMessage", [this](const std::string& topic, const std::string& message) {
            sendMessage(topic, message);
        });

        ecs.set_function("registerSystem", [this](sol::table system) {
            _systems.push_back(system);
            if (system["init"].valid()) {
                try {
                    system["init"]();
                } catch (const sol::error& e) {
                    std::cerr << "[LuaECS] Error in system init: " << e.what() << std::endl;
                }
            }
        });
    }

    void LuaECS::loadScript(const std::string& path) {
        try {
            _lua.script_file(path);
            std::cout << "[LuaECS] Loaded script: " << path << std::endl;
        } catch (const sol::error& e) {
            std::cerr << "[LuaECS] Error loading script: " << e.what() << std::endl;
        }
    }

    void LuaECS::loop() {
        auto frameDuration = std::chrono::milliseconds(16);
        for (auto& system : _systems) {
            if (system["update"].valid()) {
                try {
                    system["update"](0.016f);
                } catch (const sol::error& e) {
                    std::cerr << "[LuaECS] Error in system update: " << e.what() << std::endl;
                }
            }
        }
        std::this_thread::sleep_for(frameDuration);
    }

    void LuaECS::cleanup() {
        _systems.clear();
        _entities.clear();
        _components.clear();
    }

}

extern "C" __declspec(dllexport) rtypeEngine::IModule* createModule(const char* pubEndpoint, const char* subEndpoint) {
    return new rtypeEngine::LuaECS(pubEndpoint, subEndpoint);
}
