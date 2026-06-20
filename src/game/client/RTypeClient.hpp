#pragma once

#include "../Rtype.hpp"
#include <string>

namespace rtypeGame {

class RTypeClient : public RTypeGame {
public:
    RTypeClient(bool isLocal, const std::string& serverIp = "", int serverPort = 0,
                int simulateLagMs = 0, bool printBandwidth = false);
    ~RTypeClient() override = default;

protected:
    void onInit() override;
    void onLoop() override;

private:
    bool _isLocal;
    std::string _serverIp;
    int _serverPort;
    int _simulateLagMs;
    bool _printBandwidth;
};

} // namespace rtypeGame
