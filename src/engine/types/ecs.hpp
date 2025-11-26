#pragma once

#include <sol/sol.hpp>
#include <vector>
#include <string>
#include <unordered_map>

namespace rtypeEngine {

    struct ComponentPool {
        std::vector<sol::table> dense;
        std::vector<std::string> entities;
        std::unordered_map<std::string, size_t> sparse;
    };

}
