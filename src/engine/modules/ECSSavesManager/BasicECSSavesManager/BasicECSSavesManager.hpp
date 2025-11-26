#pragma once

#include "../IECSSavesManager.hpp"
#include <string>
#include <vector>
#include <filesystem>

namespace rtypeEngine {

    class ECSSavesManager : public IECSSavesManager {
    public:
        ECSSavesManager(const char* pubEndpoint, const char* subEndpoint);
        ~ECSSavesManager() override;

        void init() override;
        void loop() override;
        void cleanup() override;

    private:
        void createSave(const std::string& saveName, const std::string& data) override;
        void loadLastSave(const std::string& saveName) override;
        void loadFirstSave(const std::string& saveName) override;
        void getSaves(const std::string& saveName) override;

        std::string getTimestamp();
    };

}
