#pragma once

#include "../engine/app/AApplication.hpp"
#include <string>
#include <vector>

namespace rtypeGame {

class RTypeGame : public rtypeEngine::AApplication {
public:
    RTypeGame();
    virtual ~RTypeGame() = default;

    void loadModule(const std::string &moduleName);

    void init() override;

protected:
    virtual void onInit() = 0;
    virtual void onLoop() = 0;

    void loop() override;

    bool _scriptsLoaded = false;
    bool _networkInitDone = false;
};

} // namespace rtypeGame