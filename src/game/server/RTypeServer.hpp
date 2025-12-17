#pragma once

#include "../Rtype.hpp"

namespace rtypeGame {

class RTypeServer : public RTypeGame {
public:
    RTypeServer(int port);
    ~RTypeServer() override = default;

protected:
    void onInit() override;
    void onLoop() override;

private:
    int _port;
};

} // namespace rtypeGame
