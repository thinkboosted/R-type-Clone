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

  void loadScript(const std::string &path);

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

  void setupLuaBindings();
  std::string generateUuid();
};

} // namespace rtypeEngine
