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

namespace {
    void serializeToMsgPack(const sol::object& obj, msgpack::packer<msgpack::sbuffer>& pk) {
        switch (obj.get_type()) {
            case sol::type::nil:
                pk.pack_nil();
                break;
            case sol::type::boolean:
                pk.pack(obj.as<bool>());
                break;
            case sol::type::number:
                // Check if integer or float
                if (obj.is<double>()) {
                    double val = obj.as<double>();
                    // Robust check: is it effectively an integer?
                    if (std::floor(val) == val && val >= static_cast<double>(std::numeric_limits<int64_t>::min()) && val <= static_cast<double>(std::numeric_limits<int64_t>::max())) {
                        pk.pack(static_cast<int64_t>(val));
                    } else {
                        pk.pack(val);
                    }
                } else if (obj.is<int64_t>()) {
                     pk.pack(obj.as<int64_t>());
                } else {
                     pk.pack(obj.as<double>());
                }
                break;
            case sol::type::string:
                pk.pack(obj.as<std::string>());
                break;
            case sol::type::table: {
                sol::table tbl = obj.as<sol::table>();
                // Basic heuristic: if keys are 1..N sequential, it's an array. Else map.
                // For simplicity/safety in game dev, often treating everything as map is safer unless we check keys.
                // But for packed size, array is better. Let's check size.
                size_t sz = tbl.size();
                bool isArray = true;
                if (sz == 0) {
                     // Empty table, check if it has any keys
                     if (tbl.begin() != tbl.end()) isArray = false;
                } else {
                     // Check if keys are 1..sz
                     for (size_t i = 1; i <= sz; ++i) {
                         if (!tbl[i].valid()) {
                             isArray = false;
                             break;
                         }
                     }
                }

                if (isArray) {
                    pk.pack_array(sz);
                    for (size_t i = 1; i <= sz; ++i) {
                        serializeToMsgPack(tbl[i], pk);
                    }
                } else {
                    // Count keys manually for map
                    size_t mapSize = 0;
                    for (auto kv : tbl) mapSize++;
                    pk.pack_map(mapSize);
                    for (auto kv : tbl) {
                        serializeToMsgPack(kv.first, pk);
                        serializeToMsgPack(kv.second, pk);
                    }
                }
                break;
            }
            default:
                std::cerr << "[LuaECSManager] Warning: Unsupported type for MsgPack serialization" << std::endl;
                pk.pack_nil();
                break;
        }
    }

    sol::object msgpackToLua(sol::state_view& lua, const msgpack::object& obj) {
        switch (obj.type) {
            case msgpack::type::NIL:
                return sol::make_object(lua, sol::nil);
            case msgpack::type::BOOLEAN:
                return sol::make_object(lua, obj.via.boolean);
            case msgpack::type::POSITIVE_INTEGER:
                return sol::make_object(lua, obj.via.u64);
            case msgpack::type::NEGATIVE_INTEGER:
                return sol::make_object(lua, obj.via.i64);
            case msgpack::type::FLOAT32:
            case msgpack::type::FLOAT64:
                return sol::make_object(lua, obj.via.f64);
            case msgpack::type::STR:
                return sol::make_object(lua, std::string(obj.via.str.ptr, obj.via.str.size));
            case msgpack::type::BIN:
                return sol::make_object(lua, std::string(obj.via.bin.ptr, obj.via.bin.size));
            case msgpack::type::ARRAY: {
                sol::table tbl = lua.create_table();
                for (uint32_t i = 0; i < obj.via.array.size; ++i) {
                    tbl[i + 1] = msgpackToLua(lua, obj.via.array.ptr[i]);
                }
                return tbl;
            }
            case msgpack::type::MAP: {
                sol::table tbl = lua.create_table();
                for (uint32_t i = 0; i < obj.via.map.size; ++i) {
                    auto key = msgpackToLua(lua, obj.via.map.ptr[i].key);
                    auto val = msgpackToLua(lua, obj.via.map.ptr[i].val);
                    tbl[key] = val;
                }
                return tbl;
            }
            default:
                return sol::make_object(lua, sol::nil);
        }
    }
}

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

  subscribe("KeyPressed", [=](const std::string &msg) { forwardEvent("onKeyPressed", msg); });
  subscribe("KeyReleased", [=](const std::string &msg) { forwardEvent("onKeyReleased", msg); });
  subscribe("MousePressed", [=](const std::string &msg) { forwardEvent("onMousePressed", msg); });
  subscribe("MouseReleased", [=](const std::string &msg) { forwardEvent("onMouseReleased", msg); });
  subscribe("MouseMoved", [=](const std::string &msg) { forwardEvent("onMouseMoved", msg); });

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

void LuaECSManager::setupLuaBindings() {
  auto ecs = _lua.create_named_table("ECS");

  _capabilities = _lua.create_named_table("capabilities");
  ecs["capabilities"] = _capabilities;

  _capabilities["hasAuthority"] = false;
  _capabilities["hasRendering"] = false;
  _capabilities["hasLocalInput"] = false;
  _capabilities["hasNetworkSync"] = false;
  _capabilities["isLocalMode"] = false;
  _capabilities["isClientMode"] = false;
  _capabilities["isServer"] = false;

  ecs.set_function("setGameMode", [this](const std::string &mode_name) {
      if (mode_name == "SOLO") {
          _capabilities["hasAuthority"] = true;
          _capabilities["hasRendering"] = true;
          _capabilities["hasLocalInput"] = true;
          _capabilities["hasNetworkSync"] = false;
          _capabilities["isLocalMode"] = true;
          _capabilities["isClientMode"] = false;
          _capabilities["isServer"] = false;
      } else if (mode_name == "MULTI_CLIENT") {
          _capabilities["hasAuthority"] = false;
          _capabilities["hasRendering"] = true;
          _capabilities["hasLocalInput"] = true;
          _capabilities["hasNetworkSync"] = true;
          _capabilities["isLocalMode"] = false;
          _capabilities["isClientMode"] = true;
          _capabilities["isServer"] = false;
      } else if (mode_name == "MULTI_SERVER") {
          _capabilities["hasAuthority"] = true;
          _capabilities["hasRendering"] = false;
          _capabilities["hasLocalInput"] = false;
          _capabilities["hasNetworkSync"] = true;
          _capabilities["isLocalMode"] = false;
          _capabilities["isClientMode"] = false;
          _capabilities["isServer"] = true;
      } else {
          _capabilities["hasAuthority"] = false;
          _capabilities["hasRendering"] = false;
          _capabilities["hasLocalInput"] = false;
          _capabilities["hasNetworkSync"] = false;
          _capabilities["isLocalMode"] = false;
          _capabilities["isClientMode"] = false;
          _capabilities["isServer"] = false;
      }
      std::cout << "[LuaECSManager] Set game mode to: " << mode_name << std::endl;
  });

  ecs.set_function("isServer", [this]() { return _capabilities["isServer"]; });
  ecs.set_function("isLocalMode", [this]() { return _capabilities["isLocalMode"]; });
  ecs.set_function("isClientMode", [this]() { return _capabilities["isClientMode"]; });

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

      sendMessage("RenderEntityCommand", "DestroyEntity:" + id + ";");
      sendMessage("PhysicCommand", "DestroyBody:" + id + ";");
    }
  });

  ecs.set_function("createText", [this](const std::string &id,
                                        const std::string &text,
                                        const std::string &fontPath,
                                        int fontSize, bool isScreenSpace) {
    std::stringstream ss;
    ss << id << ";" << text << ";" << fontPath << ";" << fontSize << ";" << (isScreenSpace ? "1" : "0");
    std::cout << "[LuaECSManager] Sending CreateText: " << ss.str() << std::endl;
    sendMessage("CreateText", ss.str());
  });

  ecs.set_function("setText", [this](const std::string &id, const std::string &text) {
        sendMessage("RenderEntityCommand", "SetText:" + id + ":" + text);
  });

  ecs.set_function("addComponent", [this](const std::string &entityId,
                                          const std::string &componentName,
                                          sol::table componentData) {
    // std::cout << "[LuaECSManager] Adding component " << componentName << " to " << entityId << std::endl;
    if (_pools.find(componentName) == _pools.end()) {
      _pools[componentName] = ComponentPool();
    }
    ComponentPool &pool = _pools[componentName];
    try {
        if (pool.sparse.count(entityId)) {
          pool.dense[pool.sparse[entityId]] = componentData;
        } else {
          pool.dense.push_back(componentData);
          pool.entities.push_back(entityId);
          pool.sparse[entityId] = pool.dense.size() - 1;
        }

        // Automatic Renderer Sync: Create Sprite
        if (componentName == "Sprite") {
            std::string texturePath = componentData.get_or<std::string>("texture", "");
            if (!texturePath.empty()) {
                // Command: CreateEntity:Sprite:texturePath:entityId
                std::stringstream ss;
                ss << "CreateEntity:Sprite:" << texturePath << ":" << entityId;
                sendMessage("RenderEntityCommand", ss.str());
            }
        }
        // Automatic Renderer Sync: Camera
        else if (componentName == "Camera") {
             sendMessage("RenderEntityCommand", "CreateEntity:Camera:" + entityId);
             sendMessage("RenderEntityCommand", "SetActiveCamera:" + entityId);
        }

    } catch (const std::exception& e) {
        std::cerr << "[LuaECSManager] CRASH AVERTED in addComponent (" << componentName << "): " << e.what() << std::endl;
    }
  });

  ecs.set_function("syncToRenderer", [this]() {
      // 1. Sync Transforms
      auto transformPoolIt = _pools.find("Transform");
      if (transformPoolIt != _pools.end()) {
          ComponentPool& pool = transformPoolIt->second;
          for (size_t i = 0; i < pool.dense.size(); ++i) {
              std::string id = pool.entities[i];
              sol::table t = pool.dense[i];

              float x = t.get_or("x", 0.0f);
              float y = t.get_or("y", 0.0f);
              float z = t.get_or("z", 0.0f);
              float rx = t.get_or("rx", 0.0f);
              float ry = t.get_or("ry", 0.0f);
              float rz = t.get_or("rz", 0.0f);
              float sx = t.get_or("scale", 1.0f); // Simple uniform scale support
              float sy = sx;
              float sz = sx;

              // Send Position: SetPosition:id,x,y,z
              std::stringstream posSs;
              posSs << "SetPosition:" << id << "," << x << "," << y << "," << z;
              sendMessage("RenderEntityCommand", posSs.str());

              // Send Rotation: SetRotation:id,x,y,z
              // std::stringstream rotSs;
              // rotSs << "SetRotation:" << id << "," << rx << "," << ry << "," << rz;
              // sendMessage("RenderEntityCommand", rotSs.str());

              // Send Scale: SetScale:id,x,y,z
              std::stringstream scaleSs;
              scaleSs << "SetScale:" << id << "," << sx << "," << sy << "," << sz;
              sendMessage("RenderEntityCommand", scaleSs.str());
          }
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

  ecs.set_function("hasComponent", [this](const std::string &id,
                                          const std::string &name) -> bool {
    if (_pools.find(name) != _pools.end()) {
      ComponentPool &pool = _pools[name];
      return pool.sparse.count(id) > 0;
    }
    return false;
  });

  ecs.set_function("getComponent",
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
                     if (required.empty()) return {};

                     ComponentPool *smallestPool = nullptr;
                     size_t minSize = SIZE_MAX;

                     for (const auto &req : required) {
                       if (_pools.find(req) == _pools.end()) return {};
                       if (_pools[req].dense.size() < minSize) {
                         minSize = _pools[req].dense.size();
                         smallestPool = &_pools[req];
                       }
                     }
                     if (!smallestPool) return {};

                     std::vector<std::string> result;
                     for (const auto &entityId : smallestPool->entities) {
                       bool hasAll = true;
                       for (const auto &req : required) {
                         if (_pools[req].sparse.find(entityId) == _pools[req].sparse.end()) {
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

  ecs.set_function("subscribe", [this](const std::string &topic, sol::function callback) {
        if (_luaListeners.find(topic) == _luaListeners.end()) {
          subscribe(topic, [this, topic](const std::string &msg) {
            if (_luaListeners.find(topic) != _luaListeners.end()) {
              for (auto &func : _luaListeners[topic]) {
                if (func.valid()) {
                  try {
                    func(msg);
                  } catch (const sol::error &e) {
                    std::cerr << "[LuaECSManager] Error in subscriber for " << topic << ": " << e.what() << std::endl;
                  }
                }
              }
            }
          });
        }
        _luaListeners[topic].push_back(callback);
      });

  ecs.set_function("sendNetworkMessage", [this](const std::string &topic, const std::string &payload) {
    sendMessage("RequestNetworkSend", topic + " " + payload);
  });

  ecs.set_function("broadcastNetworkMessage", [this](const std::string &topic, const std::string &payload) {
        sendMessage("RequestNetworkBroadcast", topic + " " + payload);
  });

  ecs.set_function("sendToClient", [this](int clientId, const std::string &topic, const std::string &payload) {
    sendMessage("RequestNetworkSendTo", std::to_string(clientId) + " " + topic + " " + payload);
  });

  // Binary Protocol Bindings
  auto buildBinaryPayload = [](const std::string &topic, const msgpack::sbuffer &sbuf) {
      if (topic.size() > 1024) {
          throw std::runtime_error("Topic size too large (>1024)");
      }
      uint32_t topicLen = static_cast<uint32_t>(topic.size());
      const std::size_t totalSize = sizeof(topicLen) + topicLen + sbuf.size();
      std::string internalPayload(totalSize, '\0');

      char *ptr = internalPayload.data();
      std::memcpy(ptr, &topicLen, sizeof(topicLen));
      std::memcpy(ptr + sizeof(topicLen), topic.data(), topicLen);
      std::memcpy(ptr + sizeof(topicLen) + topicLen, sbuf.data(), sbuf.size());

      return internalPayload;
  };

  ecs.set_function("sendBinary", [this, buildBinaryPayload](const std::string &topic, sol::object data) {
      msgpack::sbuffer sbuf;
      msgpack::packer<msgpack::sbuffer> pk(&sbuf);
      serializeToMsgPack(data, pk);
      sendMessage("RequestNetworkSendBinary", buildBinaryPayload(topic, sbuf));
  });

  ecs.set_function("broadcastBinary", [this, buildBinaryPayload](const std::string &topic, sol::object data) {
      msgpack::sbuffer sbuf;
      msgpack::packer<msgpack::sbuffer> pk(&sbuf);
      serializeToMsgPack(data, pk);
      sendMessage("RequestNetworkBroadcastBinary", buildBinaryPayload(topic, sbuf));
  });

  ecs.set_function("sendToClientBinary", [this](int clientId, const std::string &topic, sol::object data) {
      msgpack::sbuffer sbuf;
      msgpack::packer<msgpack::sbuffer> pk(&sbuf);
      serializeToMsgPack(data, pk);

      if (topic.size() > 1024) throw std::runtime_error("Topic size too large");
      uint32_t cid = static_cast<uint32_t>(clientId);
      uint32_t topicLen = static_cast<uint32_t>(topic.size());
      size_t totalSize = 4 + 4 + topicLen + sbuf.size();
      std::string internalPayload(totalSize, '\0');

      char* ptr = internalPayload.data();
      std::memcpy(ptr, &cid, 4);
      std::memcpy(ptr + 4, &topicLen, 4);
      std::memcpy(ptr + 8, topic.data(), topicLen);
      std::memcpy(ptr + 8 + topicLen, sbuf.data(), sbuf.size());

      sendMessage("RequestNetworkSendToBinary", internalPayload);
  });

  ecs.set_function("unpackMsgPack", [this](const std::string &data) -> sol::object {
      try {
          msgpack::object_handle oh = msgpack::unpack(data.data(), data.size());
          return msgpackToLua(_lua, oh.get());
      } catch (const std::exception &e) {
          std::cerr << "[LuaECSManager] Error unpacking msgpack: " << e.what() << std::endl;
          return sol::make_object(_lua, sol::nil);
      }
  });

  ecs.set_function("splitClientIdAndMessage", [this](const std::string &data) -> std::tuple<int, std::string> {
      size_t spacePos = data.find(' ');
      if (spacePos == std::string::npos) {
          return std::make_tuple(0, data);
      }
      try {
          std::string idStr = data.substr(0, spacePos);
          int id = std::stoi(idStr);
          // Return the rest of the string after the space
          if (spacePos + 1 < data.size()) {
              return std::make_tuple(id, data.substr(spacePos + 1));
          } else {
              return std::make_tuple(id, std::string(""));
          }
      } catch (const std::exception &e) {
          std::cerr << "[LuaECSManager] Error parsing client id from message: " << e.what() << std::endl;
          return std::make_tuple(0, data);
      }
  });

  ecs.set_function("registerSystem", [this](sol::table system) {
    _systems.push_back(system);
    if (system["init"].valid()) {
      try {
        system["init"]();
      } catch (const sol::error &e) {
        std::cerr << "[LuaECSManager] Error in system init: " << e.what() << std::endl;
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

  ecs.set_function("removeSystems", [this]() {
    _systems.clear();
  });

  ecs.set_function("removeEntities", [this]() {
      for (const auto& id : _entities) {
          sendMessage("PhysicCommand", "DestroyBody:" + id + ";");
          sendMessage("RenderEntityCommand", "DestroyEntity:" + id + ";");
      }
      _entities.clear();
      _pools.clear();
  });

  std::cout << "[LuaECSManager] DEBUG: setupLuaBindings completed" << std::endl;
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

// ═══════════════════════════════════════════════════════════════
// HARD-WIRED FIXED UPDATE (Called by GameEngine at 60Hz)
// ═══════════════════════════════════════════════════════════════
void LuaECSManager::fixedUpdate(double dt) {
  // Execute all Lua systems with fixed timestep (deterministic)
  for (auto &system : _systems) {
    if (system["update"].valid()) {
      try {
        system["update"](dt);  // Pass GameEngine's fixed dt directly
      } catch (const sol::error &e) {
        std::cerr << "[LuaECSManager] Error in system update: " << e.what() << std::endl;
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════════
// LEGACY LOOP (DEPRECATED - kept for backward compatibility)
// ═══════════════════════════════════════════════════════════════
// Will be removed once all code paths use hard-wired calls
void LuaECSManager::loop() {
    // CRITICAL: Do NOT run update loop here.
    // Logic is now driven by GameEngine::executeFixedUpdate() in the main thread.
    // This thread is only responsible for processing ZMQ messages (handled in AModule::start loop).

    // Just sleep to avoid busy waiting in the thread loop
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
        if (c == '"' || c == '\\')
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
        std::cerr << "[LuaECSManager] Error serializing component: " << e.what() << std::endl;
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
        std::cerr << "[LuaECSManager] Error deserializing component: " << e.what() << std::endl;
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