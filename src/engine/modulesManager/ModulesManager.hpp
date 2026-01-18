/**
 * @file ModulesManager.hpp
 * @brief Game-specific module manager
 * 
 * @details Extends AModulesManager for the R-Type game.
 * Handles loading and lifecycle of game modules.
 * 
 * @see AModulesManager for base implementation
 */

#pragma once

#include <string>
#include <memory>
#include <vector>

#include "AModulesManager.hpp"

namespace rtypeGame {
class ModulesManager : public rtypeEngine::AModulesManager {
  public:
    ModulesManager();
};
}
