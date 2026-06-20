#pragma once

#include "../Rtype.hpp"

namespace rtypeGame {

class RTypeServer : public RTypeGame {
public:
    RTypeServer(int port, int simulateLagMs = 0, bool printBandwidth = false);
    ~RTypeServer() override = default;

protected:
    void onInit() override;
    void onLoop() override;

private:
    int _port;
    int _simulateLagMs;
    bool _printBandwidth;
};

} // namespace rtypeGame
