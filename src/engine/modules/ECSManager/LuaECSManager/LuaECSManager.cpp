#include "LuaECSManager.hpp"

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

namespace rtypeEngine {

LuaECSManager::LuaECSManager(const char *pubEndpoint, const char *subEndpoint)
    : IECSManager(pubEndpoint, subEndpoint) {}

LuaECSManager::~LuaECSManager() {}

void LuaECSManager::init() {
  _lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string,
                      sol::lib::table, sol::lib::math);

  setupLuaBindings();

  subscribe("NetworkStatus", [this](const std::string &msg) {
    if (msg.find("Bound") != std::string::npos) {
      _isServer = true;
      std::cout << "[LuaECSManager] Detected Server Mode" << std::endl;
    } else if (msg.find("Connected") != std::string::npos) {
      _isServer = false;
      std::cout << "[LuaECSManager] Detected Client Mode" << std::endl;
    }
  });

  subscribe("LoadScript",
            [this](const std::string &msg) { this->loadScript(msg); });

  subscribe("ECSStateLoadedEvent",
            [this](const std::string &msg) { this->deserializeState(msg); });

  subscribe("SavesListEvent", [this](const std::string &msg) {
    for (auto &system : _systems) {
      if (system["onSavesListReceived"].valid()) {
        try {
          system["onSavesListReceived"](msg);
        } catch (const sol::error &e) {
          std::cerr << "[LuaECSManager] Error in onSavesListReceived: "
                    << e.what() << std::endl;
        }
      }
    }
  });

  auto forwardEvent = [this](const std::string &eventName,
                             const std::string &msg) {
    for (auto &system : _systems) {
      if (system[eventName].valid()) {
        try {
          system[eventName](msg);
        } catch (const sol::error &e) {
          std::cerr << "[LuaECSManager] Error in system " << eventName << ": "
                    << e.what() << std::endl;
        }
      }
    }
  };

  subscribe("KeyPressed",
            [=](const std::string &msg) { forwardEvent("onKeyPressed", msg); });
  subscribe("KeyReleased", [=](const std::string &msg) {
    forwardEvent("onKeyReleased", msg);
  });
  subscribe("MousePressed", [=](const std::string &msg) {
    forwardEvent("onMousePressed", msg);
  });
  subscribe("MouseReleased", [=](const std::string &msg) {
    forwardEvent("onMouseReleased", msg);
  });
  subscribe("MouseMoved",
            [=](const std::string &msg) { forwardEvent("onMouseMoved", msg); });

  subscribe("PhysicEvent", [this](const std::string &msg) {
    std::stringstream ss(msg);
    std::string segment;
    while (std::getline(ss, segment, ';')) {
      if (segment.empty())
        continue;
      size_t split1 = segment.find(':');
      if (split1 == std::string::npos)
        continue;
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
                std::cerr << "[LuaECSManager] Error in onCollision: "
                          << e.what() << std::endl;
              }
            }
          }
        }
      } else if (command == "RaycastHit") {
        size_t split2 = data.find(':');
        if (split2 != std::string::npos) {
          std::string id = data.substr(0, split2);
          float distance = std::stof(data.substr(split2 + 1));
          for (auto &system : _systems) {
            if (system["onRaycastHit"].valid()) {
              try {
                system["onRaycastHit"](id, distance);
              } catch (const sol::error &e) {
                std::cerr << "[LuaECSManager] Error in onRaycastHit: "
                          << e.what() << std::endl;
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
      if (segment.empty())
        continue;
      // Format: EntityUpdated:id:x,y,z:rx,ry,rz
      size_t split1 = segment.find(':'); // After EntityUpdated
      if (split1 == std::string::npos)
        continue;

      std::string rest = segment.substr(split1 + 1);
      size_t split2 = rest.find(':'); // After id
      if (split2 == std::string::npos)
        continue;

      std::string id = rest.substr(0, split2);
      std::string coords = rest.substr(split2 + 1);

      size_t split3 = coords.find(':'); // After pos
      if (split3 == std::string::npos)
        continue;

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
            std::cerr << "[LuaECSManager] Error in onEntityUpdated: "
                      << e.what() << std::endl;
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

void LuaECSManager::setupLuaBindings() {
  auto ecs = _lua.create_named_table("ECS");

  ecs.set_function("isServer", [this]() { return _isServer; });

  ecs.set_function("createEntity", [this]() -> std::string {
    std::string id = generateUuid();
    _entities.push_back(id);
    return id;
  });

  ecs.set_function("destroyEntity", [this](const std::string &id) {
    auto it = std::find(_entities.begin(), _entities.end(), id);
    if (it != _entities.end()) {
      _entities.erase(it);

      for (auto &pair : _pools) {
        ComponentPool &pool = pair.second;
        if (pool.sparse.count(id)) {
          size_t index = pool.sparse[id];
          size_t lastIndex = pool.dense.size() - 1;
          std::string lastEntity = pool.entities[lastIndex];

          std::swap(pool.dense[index], pool.dense[lastIndex]);
          std::swap(pool.entities[index], pool.entities[lastIndex]);

          pool.sparse[lastEntity] = index;
          pool.dense.pop_back();
          pool.entities.pop_back();
          pool.sparse.erase(id);
        }
      }

      // Notify other modules
      sendMessage("RenderEntityCommand", "DestroyEntity:" + id + ";");
      sendMessage("PhysicCommand", "DestroyBody:" + id + ";");
    }
  });

  ecs.set_function("createText", [this](const std::string &id,
                                        const std::string &text,
                                        const std::string &fontPath,
                                        int fontSize, bool isScreenSpace) {
    std::stringstream ss;
    ss << "CreateText:" << id << ":" << fontPath << ":" << fontSize << ":"
       << (isScreenSpace ? "1" : "0") << ":" << text;
    sendMessage("RenderEntityCommand", ss.str());
  });

  ecs.set_function(
      "setText", [this](const std::string &id, const std::string &text) {
        sendMessage("RenderEntityCommand", "SetText:" + id + ":" + text);
      });

  ecs.set_function("addComponent", [this](const std::string &entityId,
                                          const std::string &componentName,
                                          sol::table componentData) {
    if (_pools.find(componentName) == _pools.end()) {
      _pools[componentName] = ComponentPool();
    }
    ComponentPool &pool = _pools[componentName];
    if (pool.sparse.count(entityId)) {
      pool.dense[pool.sparse[entityId]] = componentData;
    } else {
      pool.dense.push_back(componentData);
      pool.entities.push_back(entityId);
      pool.sparse[entityId] = pool.dense.size() - 1;
    }
  });

  ecs.set_function("removeComponent", [this](const std::string &id,
                                             const std::string &name) {
    if (_pools.find(name) != _pools.end()) {
      ComponentPool &pool = _pools[name];
      if (pool.sparse.count(id)) {
        size_t index = pool.sparse[id];
        size_t lastIndex = pool.dense.size() - 1;
        std::string lastEntity = pool.entities[lastIndex];

        std::swap(pool.dense[index], pool.dense[lastIndex]);
        std::swap(pool.entities[index], pool.entities[lastIndex]);

        pool.sparse[lastEntity] = index;
        pool.dense.pop_back();
        pool.entities.pop_back();
        pool.sparse.erase(id);
      }
    }
  });

  ecs.set_function(
      "getComponent",
      [this](const std::string &id, const std::string &name) -> sol::object {
        if (_pools.find(name) != _pools.end()) {
          ComponentPool &pool = _pools[name];
          if (pool.sparse.count(id)) {
            return pool.dense[pool.sparse[id]];
          }
        }
        return sol::nil;
      });

  ecs.set_function("getEntitiesWith",
                   [this](sol::table components) -> std::vector<std::string> {
                     std::vector<std::string> required;
                     for (auto &kv : components) {
                       if (kv.second.is<std::string>()) {
                         required.push_back(kv.second.as<std::string>());
                       }
                     }

                     if (required.empty())
                       return {};

                     ComponentPool *smallestPool = nullptr;
                     size_t minSize = SIZE_MAX;

                     for (const auto &req : required) {
                       if (_pools.find(req) == _pools.end())
                         return {};
                       if (_pools[req].dense.size() < minSize) {
                         minSize = _pools[req].dense.size();
                         smallestPool = &_pools[req];
                       }
                     }

                     if (!smallestPool)
                       return {};

                     std::vector<std::string> result;
                     for (const auto &entityId : smallestPool->entities) {
                       bool hasAll = true;
                       for (const auto &req : required) {
                         if (_pools[req].sparse.find(entityId) ==
                             _pools[req].sparse.end()) {
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

  ecs.set_function("sendMessage", [this](const std::string &topic,
                                         const std::string &message) {
    sendMessage(topic, message);
  });

  ecs.set_function(
      "subscribe", [this](const std::string &topic, sol::function callback) {
        if (_luaListeners.find(topic) == _luaListeners.end()) {
          subscribe(topic, [this, topic](const std::string &msg) {
            if (_luaListeners.find(topic) != _luaListeners.end()) {
              for (auto &func : _luaListeners[topic]) {
                if (func.valid()) {
                  try {
                    func(msg);
                  } catch (const sol::error &e) {
                    std::cerr << "[LuaECSManager] Error in subscriber for "
                              << topic << ": " << e.what() << std::endl;
                  }
                }
              }
            }
          });
        }
        _luaListeners[topic].push_back(callback);
      });

  ecs.set_function("sendNetworkMessage", [this](const std::string &topic,
                                                const std::string &payload) {
    sendMessage("RequestNetworkSend", topic + " " + payload);
  });

  ecs.set_function(
      "broadcastNetworkMessage",
      [this](const std::string &topic, const std::string &payload) {
        sendMessage("RequestNetworkBroadcast", topic + " " + payload);
      });

  ecs.set_function("sendToClient", [this](int clientId,
                                          const std::string &topic,
                                          const std::string &payload) {
    sendMessage("RequestNetworkSendTo",
                std::to_string(clientId) + " " + topic + " " + payload);
  });

  ecs.set_function("registerSystem", [this](sol::table system) {
    _systems.push_back(system);
    if (system["init"].valid()) {
      try {
        system["init"]();
      } catch (const sol::error &e) {
        std::cerr << "[LuaECSManager] Error in system init: " << e.what()
                  << std::endl;
      }
    }
  });

  ecs.set_function("saveState", [this](const std::string &saveName) {
    std::string state = serializeState();
    sendMessage("CreateSaveCommand", saveName + ":" + state);
  });

  ecs.set_function("loadLastSave", [this](const std::string &saveName) {
    sendMessage("LoadLastSaveCommand", saveName);
  });

  ecs.set_function("loadFirstSave", [this](const std::string &saveName) {
    sendMessage("LoadFirstSaveCommand", saveName);
  });

  ecs.set_function("getSaves", [this](const std::string &saveName) {
    sendMessage("GetSaves", saveName);
  });
}

void LuaECSManager::loadScript(const std::string &path) {
  try {
    _lua.script_file(path);
    std::cout << "[LuaECSManager] Loaded script: " << path << std::endl;
  } catch (const sol::error &e) {
    std::cerr << "[LuaECSManager] Error loading script: " << e.what()
              << std::endl;
  }
}

void LuaECSManager::loop() {
  auto frameDuration = std::chrono::milliseconds(16);
  for (auto &system : _systems) {
    if (system["update"].valid()) {
      try {
        system["update"](0.016f);
      } catch (const sol::error &e) {
        std::cerr << "[LuaECSManager] Error in system update: " << e.what()
                  << std::endl;
      }
    }
  }
  std::this_thread::sleep_for(frameDuration);
}

void LuaECSManager::cleanup() {
  _systems.clear();
  _entities.clear();
  _pools.clear();
  _luaListeners.clear();
}

std::string LuaECSManager::serializeTable(const sol::table &table) {
  std::stringstream ss;
  ss << "{";
  bool first = true;
  for (const auto &kv : table) {
    if (!first)
      ss << ",";
    first = false;

    if (kv.first.is<std::string>()) {
      ss << "[\"" << kv.first.as<std::string>() << "\"]=";
    } else if (kv.first.is<double>()) {
      ss << "[" << kv.first.as<int>() << "]=";
    }

    if (kv.second.is<std::string>()) {
      std::string str = kv.second.as<std::string>();
      ss << "\"";
      for (char c : str) {
        if (c == '\"' || c == '\\')
          ss << '\\';
        ss << c;
      }
      ss << "\"";
    } else if (kv.second.is<double>()) {
      ss << kv.second.as<double>();
    } else if (kv.second.is<bool>()) {
      ss << (kv.second.as<bool>() ? "true" : "false");
    } else if (kv.second.is<sol::table>()) {
      ss << serializeTable(kv.second.as<sol::table>());
    } else {
      ss << "nil";
    }
  }
  ss << "}";
  return ss.str();
}

std::string LuaECSManager::serializeState() {
  std::stringstream ss;
  ss << "ENTITIES:";
  for (size_t i = 0; i < _entities.size(); ++i) {
    ss << _entities[i] << (i == _entities.size() - 1 ? "" : ",");
  }
  ss << ";\n";

  for (auto &pair : _pools) {
    const std::string &poolName = pair.first;
    ComponentPool &pool = pair.second;
    ss << "POOL:" << poolName << ";\n";
    for (size_t i = 0; i < pool.dense.size(); ++i) {
      std::string entityId = pool.entities[i];
      sol::table comp = pool.dense[i];
      std::string serializedComp;
      try {
        serializedComp = serializeTable(comp);
      } catch (const std::exception &e) {
        std::cerr << "[LuaECSManager] Error serializing component: " << e.what()
                  << std::endl;
        continue;
      }
      ss << "COMP:" << entityId << ":" << serializedComp << ";\n";
    }
  }
  return ss.str();
}

void LuaECSManager::deserializeState(const std::string &state) {
  for (const auto &id : _entities) {
    sendMessage("PhysicCommand", "DestroyBody:" + id + ";");
    sendMessage("RenderEntityCommand", "DestroyEntity:" + id + ";");
  }

  _entities.clear();
  _pools.clear();

  std::stringstream ss(state);
  std::string line;
  std::string currentPoolName;

  while (std::getline(ss, line)) {
    if (line.empty())
      continue;
    if (line.back() == ';')
      line.pop_back();

    if (line.rfind("ENTITIES:", 0) == 0) {
      std::string entitiesStr = line.substr(9);
      std::stringstream ess(entitiesStr);
      std::string entityId;
      while (std::getline(ess, entityId, ',')) {
        if (!entityId.empty()) {
          _entities.push_back(entityId);
        }
      }
    } else if (line.rfind("POOL:", 0) == 0) {
      currentPoolName = line.substr(5);
    } else if (line.rfind("COMP:", 0) == 0) {
      if (currentPoolName.empty())
        continue;

      size_t split1 = line.find(':', 5);
      if (split1 == std::string::npos)
        continue;

      std::string entityId = line.substr(5, split1 - 5);
      std::string data = line.substr(split1 + 1);

      try {
        sol::table comp = _lua.script("return " + data);
        ComponentPool &pool = _pools[currentPoolName];

        pool.dense.push_back(comp);
        pool.entities.push_back(entityId);
        pool.sparse[entityId] = pool.dense.size() - 1;

      } catch (const sol::error &e) {
        std::cerr << "[LuaECSManager] Error deserializing component: "
                  << e.what() << std::endl;
      }
    }
  }
  std::cout << "[LuaECSManager] State deserialized" << std::endl;
}
} // namespace rtypeEngine

extern "C" LUA_ECS_MANAGER_EXPORT rtypeEngine::IModule *
createModule(const char *pubEndpoint, const char *subEndpoint) {
  return new rtypeEngine::LuaECSManager(pubEndpoint, subEndpoint);
}
