#pragma once

#include "../../../types/ecs.hpp"
#include "../IECSManager.hpp"
#include <map>
#include <sol/sol.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <queue>
#include <functional>

namespace rtypeEngine {

class LuaECSManager : public IECSManager {
public:
  LuaECSManager(const char *pubEndpoint, const char *subEndpoint);
  ~LuaECSManager() override;

  void init() override;
  void loop() override;
  void cleanup() override;

  // ═══════════════════════════════════════════════════════════════
  // HARD-WIRED GAME LOOP (High Performance)
  // ═══════════════════════════════════════════════════════════════
  void fixedUpdate(double /*dt*/) override;

  void loadScript(const std::string& path);
  void unloadScript(const std::string& path);
  void setSelfReference(std::shared_ptr<LuaECSManager> self);

  std::string serializeState();
  void deserializeState(const std::string &state);

  // PUBLIC ACCESSORS FOR TESTING & DEBUGGING
  sol::state& getLuaState() { return _lua; }
  std::weak_ptr<LuaECSManager> getWeakSelfRef() const { return _selfRef; }
  size_t getEntityCount() const { return _entities.size(); }
  const ComponentPool* getComponentPool(const std::string& name) const {
    auto it = _pools.find(name);
    return it != _pools.end() ? &it->second : nullptr;
  }

private:
  // Thread safety & Event Queue Management
  std::mutex _eventQueueMutex;
  std::queue<std::function<void()>> _eventQueue;
  static constexpr size_t MAX_EVENT_QUEUE_SIZE = 1000;  // Prevent unbounded growth
  static constexpr size_t EVENT_QUEUE_WARN_THRESHOLD = 800;  // Log warning when approaching limit

  // Weak reference to self for lifetime safety in callbacks
  // Prevents use-after-free when lambdas capture 'this' in Sol2 bindings
  std::weak_ptr<LuaECSManager> _selfRef;
  
  // Asset Cache Management
  struct CacheEntry {
    std::chrono::steady_clock::time_point lastUsed;
  };
  std::unordered_map<std::string, CacheEntry> _assetAccessTimes;

  std::string serializeTable(const sol::table &table);
  sol::state _lua;
  std::vector<sol::table> _systems;
  std::vector<std::string> _entities;
  std::unordered_map<std::string, ComponentPool> _pools;
  std::map<std::string, std::vector<sol::function>> _luaListeners;
  std::unordered_map<std::string, bool> _keyboardState; // Input cache
  bool _isServer = false;
  sol::table _capabilities;

  std::chrono::high_resolution_clock::time_point _lastFrameTime;
  double _accumulator = 0.0;
  const double FIXED_DT = 1.0 / 60.0;
  const double MAX_FRAME_TIME = 0.25;

  // ═══════════════════════════════════════════════════════════════
  // SERVER AUTHORITY MODEL (Multiplayer Support)
  // ═══════════════════════════════════════════════════════════════
  // Track which client owns which entity for authority checks
  std::unordered_map<std::string, int> _entityOwnership;  // entityId -> clientId (0 = server)
  int _clientId = 0;  // This client's ID (0 if server, > 0 if client)

  void setupLuaBindings();
  std::string generateUuid();
};

} // namespace rtypeEngine