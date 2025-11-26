#pragma once

#include "../IECSManager.hpp"
#include "../../../types/ecs.hpp"
#include <sol/sol.hpp>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>

namespace rtypeEngine {

    class LuaECSManager : public IECSManager {
    public:
        LuaECSManager(const char* pubEndpoint, const char* subEndpoint);
        ~LuaECSManager() override;

        void init() override;
        void loop() override;
        void cleanup() override;

        void loadScript(const std::string& path);

        std::string serializeState();
        void deserializeState(const std::string& state);

    private:
        std::string serializeTable(const sol::table& table);
        sol::state _lua;
        std::vector<sol::table> _systems;
        std::vector<std::string> _entities;
        std::unordered_map<std::string, ComponentPool> _pools;

        void setupLuaBindings();
        std::string generateUuid();
    };

}
