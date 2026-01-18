/**
 * @file IECSManager.hpp
 * @brief Interface for Entity Component System modules
 * 
 * @details Defines the contract for ECS implementations.
 * 
 * @see LuaECSManager for Lua/Sol2 implementation
 * @see docs/CHANNELS.md for channel reference
 */

#pragma once

#include "../AModule.hpp"

namespace rtypeEngine {

    class IECSManager : public AModule {
    public:
        using AModule::AModule;
        virtual ~IECSManager() = default;
    };

}
