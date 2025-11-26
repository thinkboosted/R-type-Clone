#pragma once

#include "../AModule.hpp"

namespace rtypeEngine {

    class IECSManager : public AModule {
    public:
        using AModule::AModule;
        virtual ~IECSManager() = default;
    };

}
