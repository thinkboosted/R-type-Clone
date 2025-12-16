#pragma once

#include "../../../types/ecs.hpp"
#include "../IECSManager.hpp"
#include <map>
#include <sol/sol.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace rtypeEngine {

class LuaECSManager : public IECSManager {
public:
  LuaECSManager(const char *pubEndpoint, const char *subEndpoint);
  ~LuaECSManager() override;

  void init() override;
  void loop() override;
  void cleanup() override;

        void loadScript(const std::string& path);
        void unloadScript(const std::string& path);

  std::string serializeState();
  void deserializeState(const std::string &state);

private:
  std::string serializeTable(const sol::table &table);
  sol::state _lua;
  std::vector<sol::table> _systems;
  std::vector<std::string> _entities;
  std::unordered_map<std::string, ComponentPool> _pools;
  std::map<std::string, std::vector<sol::function>> _luaListeners;
  bool _isServer = false;
  sol::table _capabilities;

  std::chrono::high_resolution_clock::time_point _lastFrameTime;
  double _accumulator = 0.0;
  const double FIXED_DT = 1.0 / 60.0;  // 60 Hz (16.666ms)
  const double MAX_FRAME_TIME = 0.25;

  void setupLuaBindings();
  std::string generateUuid();
};

} // namespace rtypeEngine
