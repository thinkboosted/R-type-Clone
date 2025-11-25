#pragma once

#include "../AModule.hpp"
#include <sol/sol.hpp>
#include <vector>
#include <string>
#include <map>

namespace rtypeEngine {

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
        std::map<std::string, std::map<std::string, sol::table>> _components;

        void setupLuaBindings();
        std::string generateUuid();
    };

}
