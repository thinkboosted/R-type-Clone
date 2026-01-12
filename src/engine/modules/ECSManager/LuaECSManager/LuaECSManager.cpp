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
    ss << "CreateText:" << id << ":" << fontPath << ":" << fontSize << ":"
       << (isScreenSpace ? "1" : "0") << ":" << text;
    sendMessage("RenderEntityCommand", ss.str());
  });

  ecs.set_function("setText", [this](const std::string &id, const std::string &text) {
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
                                           int fontSize, float r, float g, float b, int zOrder) -> std::string {
    std::string id = generateUuid();
    _entities.push_back(id);
    
    std::stringstream ss;
    ss << "CreateText:" << id << ":assets/fonts/arial.ttf:" << fontSize << ":1:" << text;
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