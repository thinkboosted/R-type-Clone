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
#include "../../../core/Logger.hpp"
#include "../../../types/Vec3.hpp"
#include "../../../types/Transform.hpp"
#include "../../../types/Collider.hpp"
#include "../../../types/Mesh.hpp"

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
                if (obj.is<double>()) {
                    double val = obj.as<double>();
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
                size_t sz = tbl.size();
                bool isArray = true;
                if (sz == 0) {
                     if (tbl.begin() != tbl.end()) isArray = false;
                } else {
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
                // Logger used instead of cerr
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
    Logger::Error(std::string("[LuaECSManager] ERROR in setupLuaBindings: ") + e.what());
  } catch (const std::exception &e) {
    Logger::Error(std::string("[LuaECSManager] ERROR in setupLuaBindings (std): ") + e.what());
  }

  // Helper to queue events for the Main Thread
  auto queueEvent = [this](std::function<void()> func) {
      std::lock_guard<std::mutex> lock(_eventQueueMutex);
      _eventQueue.push(func);
  };

  subscribe("NetworkStatus", [this, queueEvent](const std::string &msg) {
    queueEvent([this, msg]() {
        if (msg.find("Bound") != std::string::npos) {
          _isServer = true;
          _capabilities["isServer"] = true;
          _capabilities["isClientMode"] = false;
          _capabilities["isLocalMode"] = false;
          _capabilities["hasAuthority"] = true;
          _capabilities["hasRendering"] = false;
          _capabilities["hasLocalInput"] = false;
          _capabilities["hasNetworkSync"] = true;
          Logger::Info("[LuaECSManager] Detected Server Mode");
        } else if (msg.find("Connected") != std::string::npos) {
          _isServer = false;
          _capabilities["isServer"] = false;
          _capabilities["isClientMode"] = true;
          _capabilities["isLocalMode"] = false;
          _capabilities["hasAuthority"] = false;
          _capabilities["hasRendering"] = true;
          _capabilities["hasLocalInput"] = true;
          _capabilities["hasNetworkSync"] = true;
          Logger::Info("[LuaECSManager] Detected Client Mode");
        }
    });
  });

  subscribe("LoadScript", [this, queueEvent](const std::string &msg) { 
      queueEvent([this, msg]() { this->loadScript(msg); }); 
  });
  subscribe("UnloadScript", [this, queueEvent](const std::string &msg) { 
      queueEvent([this, msg]() { this->unloadScript(msg); }); 
  });

  subscribe("ECSStateLoadedEvent", [this, queueEvent](const std::string &msg) { 
      queueEvent([this, msg]() { this->deserializeState(msg); }); 
  });

  subscribe("SavesListEvent", [this, queueEvent](const std::string &msg) {
    queueEvent([this, msg]() {
        for (auto &system : _systems) {
          if (system["onSavesListReceived"].valid()) {
            try {
              system["onSavesListReceived"](msg);
            } catch (const sol::error &e) {
              Logger::Error(std::string("[LuaECSManager] Error in onSavesListReceived: ") + e.what());
            }
          }
        }
    });
  });

  auto forwardEvent = [this, queueEvent](const std::string &eventName, const std::string &msg) {
    queueEvent([this, eventName, msg]() {
        for (auto &system : _systems) {
          if (system[eventName].valid()) {
            try {
              system[eventName](msg);
            } catch (const sol::error &e) {
              Logger::Error(std::string("[LuaECSManager] Error in system ") + eventName + ": " + e.what());
            }
          }
        }
    });
  };

  subscribe("KeyPressed", [=](const std::string &msg) { forwardEvent("onKeyPressed", msg); });
  subscribe("KeyReleased", [=](const std::string &msg) { forwardEvent("onKeyReleased", msg); });
  subscribe("MousePressed", [=](const std::string &msg) { forwardEvent("onMousePressed", msg); });
  subscribe("MouseReleased", [=](const std::string &msg) { forwardEvent("onMouseReleased", msg); });
  subscribe("MouseMoved", [=](const std::string &msg) { forwardEvent("onMouseMoved", msg); });

  // Input Polling Decoupling - Cache key states
  subscribe("InputKey", [this, queueEvent](const std::string &msg) {
      queueEvent([this, msg]() {
          size_t split = msg.find(':');
          if (split != std::string::npos) {
              std::string key = msg.substr(0, split);
              std::string state = msg.substr(split + 1);
              // Store as uppercase for consistency
              std::transform(key.begin(), key.end(), key.begin(), ::toupper);
              _keyboardState[key] = (state == "1");
          }
      });
  });

  subscribe("PhysicEvent", [this, queueEvent](const std::string &msg) {
    queueEvent([this, msg]() {
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
                    Logger::Error(std::string("[LuaECSManager] Error in onCollision: ") + e.what());
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
                    Logger::Error(std::string("[LuaECSManager] Error in onRaycastHit: ") + e.what());
                  }
                }
              }
            }
          }
        }
    });
  });

  subscribe("EntityUpdated", [this, queueEvent](const std::string &msg) {
    queueEvent([this, msg]() {
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
                Logger::Error(std::string("[LuaECSManager] Error in onEntityUpdated: ") + e.what());
              }
            }
          }
        }
    });
  });

  Logger::Info("[LuaECSManager] Initialized");
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
  // Register Vec3 UserType with optimized bindings
  // OPTIMIZATION: Use lambdas for in-place operations and careful return handling
  _lua.new_usertype<Vec3>("Vec3",
      sol::constructors<Vec3(), Vec3(float, float, float)>(),
      "x", &Vec3::x,
      "y", &Vec3::y,
      "z", &Vec3::z,
      // OPTIMIZATION: normalize() now modifies in-place to avoid copies
      "normalize", [](Vec3& self) -> Vec3& {
          self = self.normalize();
          return self;
      },
      // OPTIMIZATION: length() returns primitive (no copy overhead)
      "length", [](const Vec3& self) -> float {
          return self.length();
      },
      // Arithmetic operations: must return copies (immutability constraint)
      sol::meta_function::addition, [](const Vec3& a, const Vec3& b) -> Vec3 {
          return a + b;
      },
      sol::meta_function::subtraction, [](const Vec3& a, const Vec3& b) -> Vec3 {
          return a - b;
      },
      sol::meta_function::multiplication, [](const Vec3& a, float b) -> Vec3 {
          return a * b;
      },
      sol::meta_function::division, [](const Vec3& a, float b) -> Vec3 {
          return a / b;
      },
      // NEW: Custom method for in-place add (Lua: v:addInPlace(other))
      "addInPlace", [](Vec3& self, const Vec3& other) -> Vec3& {
          self.x += other.x;
          self.y += other.y;
          self.z += other.z;
          return self;
      },
      // NEW: Custom method for in-place multiply (Lua: v:mulInPlace(scalar))
      "mulInPlace", [](Vec3& self, float scalar) -> Vec3& {
          self.x *= scalar;
          self.y *= scalar;
          self.z *= scalar;
          return self;
      },
      sol::meta_function::to_string, &Vec3::toString
  );

  // Register Transform UserType
  _lua.new_usertype<Transform>("Transform",
      sol::constructors<
          Transform(),
          Transform(float, float, float),
          Transform(float, float, float, float, float, float),
          Transform(float, float, float, float, float, float, float, float, float)
      >(),
      "x", &Transform::x,
      "y", &Transform::y,
      "z", &Transform::z,
      "rx", &Transform::rx,
      "ry", &Transform::ry,
      "rz", &Transform::rz,
      "sx", &Transform::sx,
      "sy", &Transform::sy,
      "sz", &Transform::sz,
      "setPosition", &Transform::setPosition,
      "setRotation", &Transform::setRotation,
      "setScale", &Transform::setScale,
      "getDistance", &Transform::getDistance,
      sol::meta_function::to_string, [](const Transform& self) -> std::string {
          std::stringstream ss;
          ss << "Transform(" << self.x << ", " << self.y << ", " << self.z
             << " | rx=" << self.rx << ", ry=" << self.ry << ", rz=" << self.rz
             << " | sx=" << self.sx << ", sy=" << self.sy << ", sz=" << self.sz << ")";
          return ss.str();
      }
  );

  // Register Collider UserType
  _lua.new_usertype<Collider>("Collider",
      sol::constructors<
          Collider(),
          Collider(const std::string&),
          Collider(const std::string&, float, float, float)
      >(),
      "type", sol::property(
          [](const Collider& self) { return self.getTypeString(); },
          [](Collider& self, const std::string& str) { self.type = Collider::parseType(str); }
      ),
      "size", &Collider::size,
      "setSize", [](Collider& self, float x, float y, float z) { self.size = {x, y, z}; },
      "isValid", &Collider::isValid,
      "getTypeString", &Collider::getTypeString,
      sol::meta_function::to_string, [](const Collider& self) -> std::string {
          std::stringstream ss;
          ss << "Collider(" << self.getTypeString() << ", "
             << self.size[0] << ", " << self.size[1] << ", " << self.size[2] << ")";
          return ss.str();
      }
  );

  // Register Mesh UserType
  _lua.new_usertype<Mesh>("Mesh",
      sol::constructors<
          Mesh(),
          Mesh(const std::string&),
          Mesh(const std::string&, const std::string&)
      >(),
      "modelPath", &Mesh::modelPath,
      "texturePath", &Mesh::texturePath,
      "validatePaths", &Mesh::validatePaths,
      "isModelLoaded", &Mesh::isModelLoaded,
      "isTextureLoaded", &Mesh::isTextureLoaded,
      sol::meta_function::to_string, [](const Mesh& self) -> std::string {
          std::stringstream ss;
          ss << "Mesh(\"" << self.modelPath << "\"";
          if (!self.texturePath.empty()) {
              ss << ", \"" << self.texturePath << "\"";
          }
          ss << ")";
          return ss.str();
      }
  );

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
      Logger::Info("[LuaECSManager] Set game mode to: " + mode_name);
  });

  ecs.set_function("isServer", [this]() { return _capabilities["isServer"]; });
  ecs.set_function("isLocalMode", [this]() { return _capabilities["isLocalMode"]; });
  ecs.set_function("isClientMode", [this]() { return _capabilities["isClientMode"]; });

  ecs.set_function("log", [](const std::string &message) {
      Logger::Info("[Lua] " + message);
  });

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
    Logger::Debug("[LuaECSManager] Sending CreateText: " + ss.str());
    sendMessage("CreateText", ss.str());
  });

  ecs.set_function("setText", [this](const std::string &id, const std::string &text) {
        sendMessage("RenderEntityCommand", "SetText:" + id + ":" + text);
  });

  ecs.set_function("setTexture", [this](const std::string &id, const std::string &path) {
        sendMessage("RenderEntityCommand", "SetTexture:" + id + ":" + path);
  });

    ecs.set_function("playSound", [this](const std::string &path) {
      // Fire & Forget sound effect
      sendMessage("PlaySound", path);
    });

    // ═══════════════════════════════════════════════════════════════
    // MUSIC PLAYBACK (Background music with control)
    // ═══════════════════════════════════════════════════════════════
    ecs.set_function("playMusic", [this](const std::string &musicId, 
                      const std::string &path, 
                      float volume, bool loop) {
      std::stringstream ss;
      ss << musicId << ":" << path << ":" << volume << ":" << (loop ? "1" : "0");
      sendMessage("MusicPlay", ss.str());
      Logger::Info("[LuaECSManager] Playing music: " + musicId + " from " + path);
    });

    ecs.set_function("stopMusic", [this](const std::string &musicId) {
      sendMessage("MusicStop", musicId);
    });

    ecs.set_function("pauseMusic", [this](const std::string &musicId) {
      sendMessage("MusicPause", musicId);
    });

    ecs.set_function("resumeMusic", [this](const std::string &musicId) {
      sendMessage("MusicResume", musicId);
    });

    ecs.set_function("setMusicVolume", [this](const std::string &musicId, float volume) {
      std::stringstream ss;
      ss << musicId << ":" << volume;
      sendMessage("MusicSetVolume", ss.str());
    });

    ecs.set_function("createMesh", [this](const std::string &id, const std::string &path) {
      sendMessage("RenderEntityCommand", "CreateEntity:MESH:" + path + ":" + id + ":0:0:0:0");
    });

    // ═══════════════════════════════════════════════════════════════
    // CRITICAL: Collider binding for physics (Bullet integration)
    // ═══════════════════════════════════════════════════════════════
    ecs.set_function("setCollider", [this](const std::string &id, 
                        const std::string &type,
                        float sx, float sy, float sz) {
      // Create physics body in Bullet
      std::stringstream ss;
      // Format expected by Bullet: CreateBody:<id>:<type>:<sx,sy,sz,mass,friction>;
      ss << "CreateBody:" << id << ":" << type << ":" << sx << "," << sy << "," << sz << ",1.0,0" << ";";
      sendMessage("PhysicCommand", ss.str());
      Logger::Debug("[LuaECSManager] Created collider for entity " + id + " of type " + type);
    });

    ecs.set_function("getCollider", [this](const std::string &id) -> sol::table {
      // Query collider component from pool
      sol::table result = _lua.create_table();
      if (_pools.find("Collider") != _pools.end()) {
        ComponentPool &pool = _pools["Collider"];
        if (pool.sparse.count(id)) {
          return pool.dense[pool.sparse[id]];
        }
      }
      return result;  // Empty if not found
    });

    ecs.set_function("removeCollider", [this](const std::string &id) {
      // Destroy physics body
      sendMessage("PhysicCommand", "DestroyBody:" + id + ";");
      // Remove component
      if (_pools.find("Collider") != _pools.end()) {
        ComponentPool &pool = _pools["Collider"];
        if (pool.sparse.count(id)) {
          size_t index = pool.sparse[id];
          size_t lastIndex = pool.dense.size() - 1;
          if (index != lastIndex) {
            std::string lastEntity = pool.entities[lastIndex];
            std::swap(pool.dense[index], pool.dense[lastIndex]);
            std::swap(pool.entities[index], pool.entities[lastIndex]);
            pool.sparse[lastEntity] = index;
          }
          pool.dense.pop_back();
          pool.entities.pop_back();
          pool.sparse.erase(id);
        }
      }
    });

      // ═══════════════════════════════════════════════════════════════
      // MULTIPLAYER AUTHORITY CONTROL
      // ═══════════════════════════════════════════════════════════════
      ecs.set_function("setEntityOwner", [this](const std::string &id, int clientId) {
        _entityOwnership[id] = clientId;
        Logger::Debug("[LuaECSManager] Entity " + id + " owned by client " + std::to_string(clientId));
      });

      ecs.set_function("getEntityOwner", [this](const std::string &id) -> int {
        auto it = _entityOwnership.find(id);
        if (it != _entityOwnership.end()) {
          return it->second;
        }
        return 0;  // Default to server
      });

      ecs.set_function("isEntityOwned", [this](const std::string &id) -> bool {
        return _entityOwnership.find(id) != _entityOwnership.end();
      });

      ecs.set_function("canModifyEntity", [this](const std::string &id) -> bool {
        // AUTHORITY: Check if this client can modify entity
        // Server (clientId=0) can always modify
        if (_clientId == 0 && _capabilities["isServer"]) return true;
      
        // Client can only modify own entities
        auto it = _entityOwnership.find(id);
        if (it != _entityOwnership.end()) {
          return it->second == _clientId;
        }
      
        // If not owned, only server modifies
        return false;
      });

      ecs.set_function("setClientId", [this](int clientId) {
        _clientId = clientId;
        Logger::Info("[LuaECSManager] Client ID set to " + std::to_string(clientId));
      });

      ecs.set_function("getClientId", [this]() -> int {
        return _clientId;
      });

      ecs.set_function("isKeyPressed", [this](const std::string &keyName) -> bool {
      // Convert typical Lua keys (Z, Space) to Uppercase to match KeysMap (Z, SPACE)
      std::string upperKey = keyName;
      std::transform(upperKey.begin(), upperKey.end(), upperKey.begin(), ::toupper);
      
      auto it = _keyboardState.find(upperKey);
      if (it != _keyboardState.end()) {
          return it->second;
      }
      return false;
  });

  ecs.set_function("addComponent", [this](const std::string &entityId,
                                          const std::string &componentName,
                                          sol::object componentData) {
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
            sol::table componentTable = componentData.as<sol::table>();
            std::string texturePath = componentTable.get_or<std::string>("texture", "");
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
        Logger::Error(std::string("[LuaECSManager] CRASH AVERTED in addComponent (") + componentName + "): " + e.what());
    }
  });

  ecs.set_function("updateComponent", [this](const std::string &id, const std::string &component, sol::table data) {
      if (component == "Transform") {
          // Update internal storage if needed (optional if pool is reference-based, but safe to do)
          if (_pools.find(component) != _pools.end()) {
              ComponentPool &pool = _pools[component];
              if (pool.sparse.count(id)) {
                  pool.dense[pool.sparse[id]] = data;
              }
          }

          float x = data.get_or("x", 0.0f);
          float y = data.get_or("y", 0.0f);
          float z = data.get_or("z", 0.0f);
          float rx = data.get_or("rx", 0.0f);
          float ry = data.get_or("ry", 0.0f);
          float rz = data.get_or("rz", 0.0f);
          float s = data.get_or("scale", 1.0f);

          std::stringstream posSs;
          posSs << "SetPosition:" << id << "," << x << "," << y << "," << z;
          sendMessage("RenderEntityCommand", posSs.str());

          std::stringstream rotSs;
          rotSs << "SetRotation:" << id << "," << rx << "," << ry << "," << rz;
          sendMessage("RenderEntityCommand", rotSs.str());

          std::stringstream scaleSs;
          scaleSs << "SetScale:" << id << "," << s << "," << s << "," << s;
          sendMessage("RenderEntityCommand", scaleSs.str());
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
                    Logger::Error(std::string("[LuaECSManager] Error in subscriber for ") + topic + ": " + e.what());
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

  // ═══════════════════════════════════════════════════════════════
  // NETWORK STATE SYNCHRONIZATION
  // ═══════════════════════════════════════════════════════════════
  ecs.set_function("syncEntityState", [this](const std::string &entityId, float x, float y, float z) {
      // Server broadcasts entity position to all clients
      if (_capabilities["isServer"] || _clientId == 0) {
          std::stringstream ss;
          ss << entityId << ":" << x << "," << y << "," << z;
          sendMessage("RequestNetworkBroadcast", "EntityStateSync " + ss.str());
      }
  });

  ecs.set_function("broadcastPhysicsUpdate", [this](const std::string &message) {
      // Server broadcasts physics events (collisions, forces)
      if (_capabilities["isServer"] || _clientId == 0) {
          sendMessage("RequestNetworkBroadcast", "PhysicsUpdate " + message);
      }
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
          Logger::Error(std::string("[LuaECSManager] Error unpacking msgpack: ") + e.what());
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
          Logger::Error(std::string("[LuaECSManager] Error parsing client id from message: ") + e.what());
          return std::make_tuple(0, data);
      }
  });

  ecs.set_function("registerSystem", [this](sol::table system) {
    _systems.push_back(system);
    if (system["init"].valid()) {
      try {
        system["init"]();
      } catch (const sol::error &e) {
        Logger::Error(std::string("[LuaECSManager] Error in system init: ") + e.what());
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
      _entityOwnership.clear();
    });

    // ═══════════════════════════════════════════════════════════════
    // SCENE MANAGEMENT (Level transitions)
    // ═══════════════════════════════════════════════════════════════
    ecs.set_function("loadScene", [this](const std::string &sceneName) {
      Logger::Info("[LuaECSManager] Loading scene: " + sceneName);
      
      // 1. Clean up current scene
      {
        for (const auto& id : _entities) {
          sendMessage("PhysicCommand", "DestroyBody:" + id + ";");
          sendMessage("RenderEntityCommand", "DestroyEntity:" + id + ";");
        }
        _entities.clear();
        _pools.clear();
        _entityOwnership.clear();
        _systems.clear();
        _luaListeners.clear();
      }
      
      // 2. Load new scene script
      std::string scenePath = "assets/scripts/" + sceneName + ".lua";
      try {
        _lua.script_file(scenePath);
        Logger::Info("[LuaECSManager] Scene loaded: " + scenePath);
          
        // 3. Call init if exists
        sol::function global_init = _lua["Game"]["init"];
        if (global_init && global_init.valid()) {
          global_init();
        }
      } catch (const sol::error &e) {
        Logger::Error("[LuaECSManager] Failed to load scene " + sceneName + ": " + e.what());
      }
    });

    ecs.set_function("getCurrentScene", [this]() -> std::string {
      // Return current scene name (stored in Game table if available)
      try {
        sol::table gameTable = _lua["Game"];
        if (gameTable.valid()) {
          sol::object sceneName = gameTable["_sceneName"];
          if (sceneName.valid()) {
            return sceneName.as<std::string>();
          }
        }
      } catch (...) {}
      return "unknown";
    });

  // ============================================================================ 
  // UI HELPER FUNCTIONS
  // ============================================================================ 
  
  // Create a 2D rectangle (for button backgrounds, panels, etc.)
  // Returns the entity ID
  ecs.set_function("createRect", [this](float x, float y, float width, float height, 
                                         float r, float g, float b, float a, int zOrder) -> std::string {
    std::string id = generateUuid();
    _entities.push_back(id);
    
    std::stringstream ss;
    ss << "CreateRect:" << id << ":" << x << "," << y << "," << width << "," << height 
       << ":" << r << "," << g << "," << b << "," << a << ":1";
    sendMessage("RenderEntityCommand", ss.str());
    
    // Set z-order
    std::stringstream zss;
    zss << "SetZOrder:" << id << ":" << zOrder;
    sendMessage("RenderEntityCommand", zss.str());
    
    return id;
  });

  // Update rectangle position and size
  ecs.set_function("setRect", [this](const std::string& id, float x, float y, float width, float height) {
    std::stringstream ss;
    ss << "SetRect:" << id << ":" << x << "," << y << "," << width << "," << height;
    sendMessage("RenderEntityCommand", ss.str());
  });

  // Set rectangle/element alpha
  ecs.set_function("setAlpha", [this](const std::string& id, float alpha) {
    std::stringstream ss;
    ss << "SetAlpha:" << id << ":" << alpha;
    sendMessage("RenderEntityCommand", ss.str());
  });

  // Set element z-order (higher = rendered on top)
  ecs.set_function("setZOrder", [this](const std::string& id, int zOrder) {
    std::stringstream ss;
    ss << "SetZOrder:" << id << ":" << zOrder;
    sendMessage("RenderEntityCommand", ss.str());
  });

  // Create a screen-space text element
  // Returns the entity ID
  ecs.set_function("createUIText", [this](const std::string& text, float x, float y, 
                                           int fontSize, float r, float g, float b, int zOrder, sol::optional<std::string> fontPath) -> std::string {
    std::string id = generateUuid();
    _entities.push_back(id);
    
    std::string actualFont = fontPath.value_or("assets/fonts/arial.ttf");
    if (actualFont.empty()) actualFont = "assets/fonts/arial.ttf";
    
    std::stringstream ss;
    ss << "CreateText:" << id << ":" << actualFont << ":" << fontSize << ":1:" << text;
    sendMessage("RenderEntityCommand", ss.str());
    
    // Set position
    std::stringstream pss;
    pss << "SetPosition:" << id << "," << x << "," << y << ",0";
    sendMessage("RenderEntityCommand", pss.str());
    
    // Set color
    std::stringstream css;
    css << "SetColor:" << id << "," << r << "," << g << "," << b;
    sendMessage("RenderEntityCommand", css.str());
    
    // Set z-order
    std::stringstream zss;
    zss << "SetZOrder:" << id << ":" << zOrder;
    sendMessage("RenderEntityCommand", zss.str());
    
    return id;
  });

  // Update UI text content
  ecs.set_function("setUIText", [this](const std::string& id, const std::string& text) {
    std::stringstream ss;
    ss << "SetText:" << id << ":" << text;
    sendMessage("RenderEntityCommand", ss.str());
  });

  // Set UI element position (for text, sprites, etc.)
  ecs.set_function("setUIPosition", [this](const std::string& id, float x, float y) {
    std::stringstream ss;
    ss << "SetPosition:" << id << "," << x << "," << y << ",0";
    sendMessage("RenderEntityCommand", ss.str());
  });

  // Set UI element color
  ecs.set_function("setUIColor", [this](const std::string& id, float r, float g, float b) {
    std::stringstream ss;
    ss << "SetColor:" << id << "," << r << "," << g << "," << b;
    sendMessage("RenderEntityCommand", ss.str());
  });

  // Destroy a UI element
  ecs.set_function("destroyUI", [this](const std::string& id) {
    sendMessage("RenderEntityCommand", "DestroyEntity:" + id + ";");
    auto it = std::find(_entities.begin(), _entities.end(), id);
    if (it != _entities.end()) {
      _entities.erase(it);
    }
  });

  // Get window/screen dimensions (returns a table with width and height)
  ecs.set_function("getScreenSize", [this]() -> sol::table {
    sol::table result = _lua.create_table();
    result["width"] = 800;   // Default, should be updated from WindowResized events
    result["height"] = 600;
    return result;
  });

  // Create a circle UI element
  // ECS.createCircle(x, y, radius, r, g, b, a, zOrder, segments)
  ecs.set_function("createCircle", [this](float x, float y, float radius, 
                                           float r, float g, float b, float a, 
                                           int zOrder, sol::optional<int> segments) -> std::string {
    std::string id = generateUuid();
    _entities.push_back(id);
    
    int segs = segments.value_or(32);
    
    std::stringstream ss;
    ss << "CreateCircle:" << id << ":" << x << "," << y << "," << radius << ":"
       << r << "," << g << "," << b << "," << a << ":1:" << segs;
    sendMessage("RenderEntityCommand", ss.str());
    
    std::stringstream zss;
    zss << "SetZOrder:" << id << ":" << zOrder;
    sendMessage("RenderEntityCommand", zss.str());
    
    return id;
  });

  // Create a rounded rectangle UI element
  // ECS.createRoundedRect(x, y, width, height, cornerRadius, r, g, b, a, zOrder)
  ecs.set_function("createRoundedRect", [this](float x, float y, float width, float height,
                                                float cornerRadius, float r, float g, float b, 
                                                float a, int zOrder) -> std::string {
    std::string id = generateUuid();
    _entities.push_back(id);
    
    std::stringstream ss;
    ss << "CreateRoundedRect:" << id << ":" << x << "," << y << "," << width << "," 
       << height << "," << cornerRadius << ":" << r << "," << g << "," << b << "," << a << ":1";
    sendMessage("RenderEntityCommand", ss.str());
    
    std::stringstream zss;
    zss << "SetZOrder:" << id << ":" << zOrder;
    sendMessage("RenderEntityCommand", zss.str());
    
    return id;
  });

  // Create a line UI element
  // ECS.createLine(x1, y1, x2, y2, lineWidth, r, g, b, a, zOrder)
  ecs.set_function("createLine", [this](float x1, float y1, float x2, float y2, 
                                         float lineWidth, float r, float g, float b, 
                                         float a, int zOrder) -> std::string {
    std::string id = generateUuid();
    _entities.push_back(id);
    
    std::stringstream ss;
    ss << "CreateLine:" << id << ":" << x1 << "," << y1 << "," << x2 << "," << y2 << "," 
       << lineWidth << ":" << r << "," << g << "," << b << "," << a << ":1";
    sendMessage("RenderEntityCommand", ss.str());
    
    std::stringstream zss;
    zss << "SetZOrder:" << id << ":" << zOrder;
    sendMessage("RenderEntityCommand", zss.str());
    
    return id;
  });

  // Create a UI sprite (image)
  // ECS.createUISprite(texturePath, x, y, width, height, zOrder)
  ecs.set_function("createUISprite", [this](const std::string& texturePath, float x, float y, 
                                             float width, float height, int zOrder) -> std::string {
    std::string id = generateUuid();
    _entities.push_back(id);
    
    std::stringstream ss;
    ss << "CreateUISprite:" << id << ":" << texturePath << ":" << x << "," << y << "," 
       << width << "," << height << ":1:" << zOrder;
    sendMessage("RenderEntityCommand", ss.str());
    
    return id;
  });

  // Set outline for a UI element (rect, circle, rounded rect)
  // ECS.setOutline(id, enabled, width, r, g, b)
  ecs.set_function("setOutline", [this](const std::string& id, bool enabled, 
                                         float width, float r, float g, float b) {
    std::stringstream ss;
    ss << "SetOutline:" << id << ":" << (enabled ? "1" : "0") << ":" << width << ":" 
       << r << "," << g << "," << b;
    sendMessage("RenderEntityCommand", ss.str());
  });

  // Set corner radius for rounded rectangles
  // ECS.setCornerRadius(id, radius)
  ecs.set_function("setCornerRadius", [this](const std::string& id, float radius) {
    std::stringstream ss;
    ss << "SetCornerRadius:" << id << ":" << radius;
    sendMessage("RenderEntityCommand", ss.str());
  });

  // Set radius for circles
  // ECS.setCircleRadius(id, radius)
  ecs.set_function("setCircleRadius", [this](const std::string& id, float radius) {
    std::stringstream ss;
    ss << "SetRadius:" << id << ":" << radius;
    sendMessage("RenderEntityCommand", ss.str());
  });

  // Set UI element size (for sprites, rects)
  // ECS.setUISize(id, width, height)
  ecs.set_function("setUISize", [this](const std::string& id, float width, float height) {
    std::stringstream ss;
    ss << "SetScale:" << id << "," << width << "," << height << ",1";
    sendMessage("RenderEntityCommand", ss.str());
  });

  // Set UI sprite texture
  // ECS.setUITexture(id, texturePath)
  ecs.set_function("setUITexture", [this](const std::string& id, const std::string& texturePath) {
    std::stringstream ss;
    ss << "SetTexture:" << id << ":" << texturePath;
    sendMessage("RenderEntityCommand", ss.str());
  });

  // ============================================================================ 
  // WINDOW CONTROL FUNCTIONS
  // ============================================================================ 

  // Set fullscreen mode
  // ECS.setFullscreen(enabled)
  ecs.set_function("setFullscreen", [this](bool enabled) {
    sendMessage("SetFullscreen", enabled ? "1" : "0");
  });

  // Toggle fullscreen mode
  // ECS.toggleFullscreen()
  ecs.set_function("toggleFullscreen", [this]() {
    sendMessage("ToggleFullscreen", "");
  });

  // Set window size (only works in windowed mode)
  // ECS.setWindowSize(width, height)
  ecs.set_function("setWindowSize", [this](int width, int height) {
    std::stringstream ss;
    ss << width << "," << height;
    std::cout << "[LuaECSManager] setWindowSize called: " << width << "x" << height << std::endl << std::flush;
    sendMessage("SetWindowSize", ss.str());
    std::cout << "[LuaECSManager] setWindowSize message sent" << std::endl << std::flush;
  });

  // Request window info (will trigger WindowInfo event)
  // ECS.requestWindowInfo()
  ecs.set_function("requestWindowInfo", [this]() {
    sendMessage("GetWindowInfo", "");
  });

  // Close the window/exit application
  // ECS.closeWindow()
  ecs.set_function("closeWindow", [this]() {
    sendMessage("CloseWindow", "");
  });

  Logger::Debug("[LuaECSManager] DEBUG: setupLuaBindings completed");
}

void LuaECSManager::setSelfReference(std::shared_ptr<LuaECSManager> self) {
    _selfRef = self;
}

void LuaECSManager::loadScript(const std::string &path) {
  try {
    sol::table ecsGlobal = _lua.globals()["ECS"];
    _lua.script_file(path);
    Logger::Info("[LuaECSManager] Loaded script: " + path);
  } catch (const sol::error &e) {
    Logger::Error(std::string("[LuaECSManager] Error loading script: ") + e.what());
  }
}

void LuaECSManager::unloadScript(const std::string& path) {
    try {
        // Destroy all networked entities with messages
        for (const auto& id : _entities) {
            sendMessage("PhysicCommand", "DestroyBody:" + id + ";");
            sendMessage("RenderEntityCommand", "DestroyEntity:" + id + ";");
        }

        // Clear internal state WITHOUT destroying the Lua state
        // This prevents dangling references to _capabilities table and other Sol2 objects
        _systems.clear();
        _entities.clear();
        _pools.clear();
        _luaListeners.clear();

        // Instead of: _lua = sol::state(); (which destroys all references)
        // We selectively clear only the entity/system data via Lua:
        try {
            sol::table ecs = _lua["ECS"];
            if (ecs.valid()) {
                // Clear user-defined systems and data but keep binding functions
                ecs["systems"] = _lua.create_table();  // Reset systems table
                // Reset game mode to default
                sol::table caps = ecs["capabilities"];
                if (caps.valid()) {
                    caps["hasAuthority"] = false;
                    caps["hasRendering"] = false;
                    caps["hasLocalInput"] = false;
                    caps["hasNetworkSync"] = false;
                    caps["isLocalMode"] = false;
                    caps["isClientMode"] = false;
                    caps["isServer"] = false;
                }
            }
        } catch (const sol::error& e) {
            Logger::Info(std::string("[LuaECSManager] Info clearing Lua state: ") + e.what());
        }

        Logger::Info("[LuaECSManager] Unloaded scripts and cleared ECS state");
    } catch (const std::exception& e) {
        Logger::Error(std::string("[LuaECSManager] Error unloading scripts: ") + e.what());
    }
}

// ═══════════════════════════════════════════════════════════════
// HARD-WIRED FIXED UPDATE (Called by GameEngine at 60Hz)
// ═══════════════════════════════════════════════════════════════
void LuaECSManager::fixedUpdate(double dt) {
  // 1. Process queued events (e.g. Input, Script Load) from Main Thread
  // OPTIMIZATION: Limit queue processing per frame to prevent frame stalls
  {
      std::lock_guard<std::mutex> lock(_eventQueueMutex);
      
      // Log warning if queue is growing (potential backlog)
      size_t queueSize = _eventQueue.size();
      if (queueSize > EVENT_QUEUE_WARN_THRESHOLD) {
          Logger::Info("[LuaECSManager] WARNING: Event queue size = " + std::to_string(queueSize) + 
                      "/" + std::to_string(MAX_EVENT_QUEUE_SIZE));
      }
      
      // Process at most 32 events per frame to prevent stalling
      const int MAX_EVENTS_PER_FRAME = 32;
      int processed = 0;
      while (!_eventQueue.empty() && processed < MAX_EVENTS_PER_FRAME) {
          auto func = _eventQueue.front();
          _eventQueue.pop();
          if (func) {
              try {
                  func();
              } catch (const std::exception& e) {
                  Logger::Error("[LuaECSManager] Error processing queued event: " + std::string(e.what()));
              }
          }
          ++processed;
      }
      
      // SAFETY: If queue still growing, discard oldest to prevent memory leak
      if (queueSize >= MAX_EVENT_QUEUE_SIZE) {
          Logger::Error("[LuaECSManager] Event queue FULL! Discarding " + 
                       std::to_string(queueSize - MAX_EVENT_QUEUE_SIZE + 1) + " oldest events");
          while (_eventQueue.size() > MAX_EVENT_QUEUE_SIZE - 100) {
              _eventQueue.pop();  // Discard oldest
          }
      }
  }

  // 2. Execute all Lua systems with fixed timestep (deterministic)
  // OPTIMIZATION: Cache number of systems to avoid repeated .size() calls
  const size_t systemCount = _systems.size();
  for (size_t i = 0; i < systemCount; ++i) {
    auto &system = _systems[i];
    if (system["update"].valid()) {
      try {
        system["update"](dt);  // Pass GameEngine's fixed dt directly
      } catch (const sol::error &e) {
        Logger::Error(std::string("[LuaECSManager] Error in system update: ") + e.what());
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
        Logger::Error(std::string("[LuaECSManager] Error serializing component: ") + e.what());
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
        Logger::Error(std::string("[LuaECSManager] Error deserializing component: ") + e.what());
      }
    }
  }
  Logger::Info("[LuaECSManager] State deserialized");
}
} // namespace rtypeEngine

extern "C" LUA_ECS_MANAGER_EXPORT rtypeEngine::IModule *
createModule(const char *pubEndpoint, const char *subEndpoint) {
  return new rtypeEngine::LuaECSManager(pubEndpoint, subEndpoint);
}
