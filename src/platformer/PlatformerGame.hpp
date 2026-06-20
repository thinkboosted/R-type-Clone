#pragma once

#include "../engine/app/AApplication.hpp"
#include <string>

namespace platformerGame {

enum class PlatformerMode {
    Solo,
    Host,
    Client
};

class PlatformerGame : public rtypeEngine::AApplication {
public:
    PlatformerGame(PlatformerMode mode = PlatformerMode::Solo,
                   const std::string &serverIp = "127.0.0.1", int serverPort = 1234);
    virtual ~PlatformerGame() = default;

    void loadModule(const std::string &moduleName);

    void init() override;

protected:
    void loop() override;

private:
    bool _scriptsLoaded = false;
    bool _networkInitDone = false;
    PlatformerMode _mode;
    std::string _serverIp;
    int _serverPort;
};

} // namespace platformerGame
