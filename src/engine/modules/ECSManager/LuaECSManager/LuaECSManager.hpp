/**
 * @file LuaECSManager.hpp
 * @brief Lua-based Entity Component System module
 * 
 * @details Hosts the game logic via Lua scripting using Sol2.
 * Manages entities, components, and systems defined in Lua scripts.
 * Acts as the central game logic hub, forwarding commands to other modules.
 * 
 * @section channels_sub Subscribed Channels (forwarded to Lua)
 * | Channel | Source | Description |
 * |---------|--------|-------------|
 * | `KeyPressed` | WindowManager | Key press events |
 * | `KeyReleased` | WindowManager | Key release events |
 * | `MousePressed` | WindowManager | Mouse click events |
 * | `MouseMoved` | WindowManager | Mouse movement |
 * | `Collision` | PhysicEngine | Physics collision events |
 * | `NetworkMessage` | NetworkManager | Network messages |
 * | Custom topics | Various | Game-specific events |
 * 
 * @section channels_pub Published Channels (from Lua)
 * | Channel | Target | Description |
 * |---------|--------|-------------|
 * | `RenderEntityCommand` | Renderer | Rendering instructions |
 * | `PhysicCommand` | PhysicEngine | Physics commands |
 * | `SoundPlay` | SoundManager | Play sound effects |
 * | `MusicPlay` | SoundManager | Play music |
 * | `RequestNetworkSend` | NetworkManager | Send network message |
 * | `ExitApplication` | Application | Exit the application |
 * 
 * @section lua_api Lua API
 * The ECS exposes these functions to Lua:
 * - `ECS.createEntity()` - Create new entity
 * - `ECS.destroyEntity(id)` - Remove entity
 * - `ECS.addComponent(id, name, data)` - Add component
 * - `ECS.getComponent(id, name)` - Get component
 * - `ECS.getEntitiesWith({components})` - Query entities
 * - `ECS.subscribe(topic, handler)` - Subscribe to channel
 * - `ECS.sendMessage(topic, payload)` - Publish message
 * - `ECS.registerSystem(system)` - Register system table
 * 
 * @see docs/CHANNELS.md for complete channel reference
 * @see assets/scripts/ for Lua game scripts
 */

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
