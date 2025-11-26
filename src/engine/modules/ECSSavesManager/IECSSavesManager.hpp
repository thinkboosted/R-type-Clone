#pragma once

#include "../AModule.hpp"
#include <string>

namespace rtypeEngine {

    class IECSSavesManager : public AModule {
    public:
        using AModule::AModule;
        virtual ~IECSSavesManager() = default;

    protected:
        virtual void createSave(const std::string& saveName, const std::string& data) = 0;
        virtual void loadLastSave(const std::string& saveName) = 0;
        virtual void loadFirstSave(const std::string& saveName) = 0;
        virtual void getSaves(const std::string& saveName) = 0;
    };

}
