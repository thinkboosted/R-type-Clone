#pragma once

#include "../Rtype.hpp"
#include <string>

namespace rtypeGame {

class RTypeClient : public RTypeGame {
public:
    RTypeClient(bool isLocal, const std::string& serverIp = "", int serverPort = 0);
    ~RTypeClient() override = default;

protected:
    void onInit() override;
    void onLoop() override;

private:
    bool _isLocal;
    std::string _serverIp;
    int _serverPort;
};

} // namespace rtypeGame
