#pragma once

#include "../AModule.hpp"
#include <sol/sol.hpp>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>

namespace rtypeEngine {

    struct ComponentPool {
        std::vector<sol::table> dense;
        std::vector<std::string> entities;
        std::unordered_map<std::string, size_t> sparse;
    };

    class LuaECS : public AModule {
    public:
        LuaECS(const char* pubEndpoint, const char* subEndpoint);
        ~LuaECS() override;

        void init() override;
        void loop() override;
        void cleanup() override;

        void loadScript(const std::string& path);

    private:
        sol::state _lua;
        std::vector<sol::table> _systems;
        std::vector<std::string> _entities;
        std::unordered_map<std::string, ComponentPool> _pools;

        void setupLuaBindings();
        std::string generateUuid();
    };

}
