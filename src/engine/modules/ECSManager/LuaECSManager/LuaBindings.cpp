#include "LuaECSManager.hpp"
#include "MsgPackUtils.hpp"
#include <msgpack.hpp>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace rtypeEngine {

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
    } catch (const std::exception& e) {
        std::cerr << "[LuaECSManager] CRASH AVERTED in addComponent (" << componentName << "): " << e.what() << std::endl;
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

  // Toggle fullscreen mode
  // ECS.toggleFullscreen()
  ecs.set_function("toggleFullscreen", [this]() {
    std::cout << "[LuaECSManager] toggleFullscreen called" << std::endl;
    sendMessage("ToggleFullscreen", "");
  });

  // Set fullscreen mode
  // ECS.setFullscreen(enabled)
  ecs.set_function("setFullscreen", [this](bool enabled) {
    std::cout << "[LuaECSManager] setFullscreen called: " << (enabled ? "true" : "false") << std::endl;
    sendMessage("SetFullscreen", enabled ? "1" : "0");
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

  std::cout << "[LuaECSManager] DEBUG: setupLuaBindings completed" << std::endl;
}

} // namespace rtypeEngine
