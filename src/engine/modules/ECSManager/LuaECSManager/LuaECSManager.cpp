#include "LuaECSManager.hpp"
#include <msgpack.hpp>

#ifdef _WIN32
#define LUA_ECS_MANAGER_EXPORT __declspec(dllexport)
#else
#define LUA_ECS_MANAGER_EXPORT
#endif
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

namespace rtypeEngine {

LuaECSManager::LuaECSManager(const char *pubEndpoint, const char *subEndpoint)
    : IECSManager(pubEndpoint, subEndpoint) {
  _lastFrameTime = std::chrono::high_resolution_clock::now();
}

LuaECSManager::~LuaECSManager() {}

void LuaECSManager::init() {
  _lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string,
                      sol::lib::table, sol::lib::math, sol::lib::io, sol::lib::os);

  try {
    setupLuaBindings();
  } catch (const sol::error &e) {
    std::cerr << "[LuaECSManager] ERROR in setupLuaBindings: " << e.what() << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "[LuaECSManager] ERROR in setupLuaBindings (std): " << e.what() << std::endl;
  }

  subscribe("NetworkStatus", [this](const std::string &msg) {
    if (msg.find("Bound") != std::string::npos) {
      _isServer = true;
      _capabilities["isServer"] = true;
      _capabilities["isClientMode"] = false;
      _capabilities["isLocalMode"] = false;
      _capabilities["hasAuthority"] = true;
      _capabilities["hasRendering"] = false;
      _capabilities["hasLocalInput"] = false;
      _capabilities["hasNetworkSync"] = true;
      std::cout << "[LuaECSManager] Detected Server Mode" << std::endl;
    } else if (msg.find("Connected") != std::string::npos) {
      _isServer = false;
      _capabilities["isServer"] = false;
      _capabilities["isClientMode"] = true;
      _capabilities["isLocalMode"] = false;
      _capabilities["hasAuthority"] = false;
      _capabilities["hasRendering"] = true;
      _capabilities["hasLocalInput"] = true;
      _capabilities["hasNetworkSync"] = true;
      std::cout << "[LuaECSManager] Detected Client Mode" << std::endl;
    }
  });

  subscribe("LoadScript", [this](const std::string &msg) { this->loadScript(msg); });
  subscribe("UnloadScript", [this](const std::string &msg) { this->unloadScript(msg); });

  subscribe("ECSStateLoadedEvent", [this](const std::string &msg) { this->deserializeState(msg); });

  subscribe("SavesListEvent", [this](const std::string &msg) {
    for (auto &system : _systems) {
      if (system["onSavesListReceived"].valid()) {
        try {
          system["onSavesListReceived"](msg);
        } catch (const sol::error &e) {
          std::cerr << "[LuaECSManager] Error in onSavesListReceived: " << e.what() << std::endl;
        }
      }
    }
  });

  auto forwardEvent = [this](const std::string &eventName, const std::string &msg) {
    for (auto &system : _systems) {
      if (system[eventName].valid()) {
        try {
          system[eventName](msg);
        } catch (const sol::error &e) {
          std::cerr << "[LuaECSManager] Error in system " << eventName << ": " << e.what() << std::endl;
        }
      }
    }
  };

  // Pre-subscribe to standard events to avoid AModule iterator invalidation (crash) during LoadScript
  // and to support explicit ECS.subscribe calls without double-dispatch.
  std::vector<std::string> inputEvents = {"KeyPressed", "KeyReleased", "MousePressed", "MouseReleased", "MouseMoved", "WindowResized"};
  for (const auto& topic : inputEvents) {
      // Create empty entry so subsequent ECS.subscribe calls don't trigger AModule::subscribe
      _luaListeners[topic] = {};

      subscribe(topic, [this, topic](const std::string &msg) {
          if (_luaListeners.find(topic) != _luaListeners.end()) {
              for (auto &func : _luaListeners[topic]) {
                  if (func.valid()) {
                      try {
                          func(msg);
                      } catch (const sol::error &e) {
                          std::cerr << "[LuaECSManager] Error in " << topic << " listener: " << e.what() << std::endl;
                      }
                  }
              }
          }
      });
  }

  subscribe("PhysicEvent", [this](const std::string &msg) {
    std::stringstream ss(msg);
    std::string segment;
    while (std::getline(ss, segment, ';')) {
      if (segment.empty()) continue;
      size_t split1 = segment.find(':');
      if (split1 == std::string::npos) continue;
      std::string command = segment.substr(0, split1);
      std::string data = segment.substr(split1 + 1);

      if (command == "Collision") {
        for (auto &system : _systems) {
          if (system["onCollision"].valid()) {
            size_t split2 = data.find(':');
            if (split2 != std::string::npos) {
              std::string id1 = data.substr(0, split2);
              std::string id2 = data.substr(split2 + 1);
              try {
                system["onCollision"](id1, id2);
              } catch (const sol::error &e) {
                std::cerr << "[LuaECSManager] Error in onCollision: " << e.what() << std::endl;
              }
            }
          }
        }
      } else if (command == "RaycastHit") {
        size_t split2 = data.find(':');
        if (split2 != std::string::npos) {
          std::string id = data.substr(0, split2);
          float distance = 0.0f;
          try {
             distance = std::stof(data.substr(split2 + 1));
          } catch (...) { distance = 0.0f; }
          for (auto &system : _systems) {
            if (system["onRaycastHit"].valid()) {
              try {
                system["onRaycastHit"](id, distance);
              } catch (const sol::error &e) {
                std::cerr << "[LuaECSManager] Error in onRaycastHit: " << e.what() << std::endl;
              }
            }
          }
        }
      }
    }
  });

  subscribe("EntityUpdated", [this](const std::string &msg) {
    std::stringstream ss(msg);
    std::string segment;
    while (std::getline(ss, segment, ';')) {
      if (segment.empty()) continue;
      size_t split1 = segment.find(':');
      if (split1 == std::string::npos) continue;
      std::string rest = segment.substr(split1 + 1);
      size_t split2 = rest.find(':');
      if (split2 == std::string::npos) continue;
      std::string id = rest.substr(0, split2);
      std::string coords = rest.substr(split2 + 1);
      size_t split3 = coords.find(':');
      if (split3 == std::string::npos) continue;

      std::string posStr = coords.substr(0, split3);
      std::string rotStr = coords.substr(split3 + 1);

      float x, y, z, rx, ry, rz;
      char comma;
      std::stringstream pss(posStr);
      pss >> x >> comma >> y >> comma >> z;
      std::stringstream rss(rotStr);
      rss >> rx >> comma >> ry >> comma >> rz;

      for (auto &system : _systems) {
        if (system["onEntityUpdated"].valid()) {
          try {
            system["onEntityUpdated"](id, x, y, z, rx, ry, rz);
          } catch (const sol::error &e) {
            std::cerr << "[LuaECSManager] Error in onEntityUpdated: " << e.what() << std::endl;
          }
        }
      }
    }
  });

  std::cout << "[LuaECSManager] Initialized" << std::endl;
}

std::string LuaECSManager::generateUuid() {
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

void LuaECSManager::loadScript(const std::string &path) {
  try {
    sol::table ecsGlobal = _lua.globals()["ECS"];
    _lua.script_file(path);
    std::cout << "[LuaECSManager] Loaded script: " << path << std::endl;
  } catch (const sol::error &e) {
    std::cerr << "[LuaECSManager] Error loading script: " << e.what() << std::endl;
  }
}

void LuaECSManager::unloadScript(const std::string& path) {
    try {
        for (const auto& id : _entities) {
            sendMessage("PhysicCommand", "DestroyBody:" + id + ";");
            sendMessage("RenderEntityCommand", "DestroyEntity:" + id + ";");
        }

        _systems.clear();
        _entities.clear();
        _pools.clear();

        _lua = sol::state();
        _lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::table, sol::lib::math, sol::lib::io, sol::lib::os);

        setupLuaBindings();

        std::cout << "[LuaECSManager] Unloaded scripts and cleared ECS state" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[LuaECSManager] Error unloading scripts: " << e.what() << std::endl;
    }
}

void LuaECSManager::loop() {
  auto currentTime = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> frameTime = currentTime - _lastFrameTime;
  _lastFrameTime = currentTime;

  double deltaTime = frameTime.count();
  if (deltaTime > MAX_FRAME_TIME) {
    deltaTime = MAX_FRAME_TIME;
  }

  _accumulator += deltaTime;

  while (_accumulator >= FIXED_DT) {
    for (auto &system : _systems) {
      if (system["update"].valid()) {
        try {
          system["update"](FIXED_DT);
        } catch (const sol::error &e) {
          std::cerr << "[LuaECSManager] Error in system update: " << e.what() << std::endl;
        }
      }
    }

    _accumulator -= FIXED_DT;
  }

  auto sleepTime = std::chrono::milliseconds(10);
  std::this_thread::sleep_for(sleepTime);
}

void LuaECSManager::cleanup() {
  _systems.clear();
  _entities.clear();
  _pools.clear();
  _luaListeners.clear();
}

} // namespace rtypeEngine

extern "C" LUA_ECS_MANAGER_EXPORT rtypeEngine::IModule *
createModule(const char *pubEndpoint, const char *subEndpoint) {
  return new rtypeEngine::LuaECSManager(pubEndpoint, subEndpoint);
}
