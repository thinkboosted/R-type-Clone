#pragma once

#include "../engine/app/AApplication.hpp"
#include <string>

namespace platformerGame {

class PlatformerGame : public rtypeEngine::AApplication {
public:
    PlatformerGame();
    virtual ~PlatformerGame() = default;

    void loadModule(const std::string &moduleName);

    void init() override;

protected:
    void loop() override;

private:
    bool _scriptsLoaded = false;
};

} // namespace platformerGame
