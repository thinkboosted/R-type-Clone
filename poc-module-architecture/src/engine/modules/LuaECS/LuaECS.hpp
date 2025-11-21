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

        void loadComponentScript(const std::string& path);
        void loadSystemScript(const std::string& path);

    private:
        sol::state _lua;
        std::vector<sol::table> _systems;

        void setupLuaBindings();
        void registerComponent(const std::string& name, const sol::table& componentDef);
    };

}
