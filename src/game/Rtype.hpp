
#pragma once

#include "../engine/app/AApplication.hpp"
#include <string>
#include <vector>

namespace rtypeGame {
class Rtype : public rtypeEngine::AApplication {
  public:
    Rtype(const std::string &endpoint, const std::vector<std::string>& args);
    void init() override;
    void loop() override;

  private:
    bool _scriptsLoaded = false;
    std::vector<std::string> _args;
    bool _networkInitDone = false;
};
}  // namespace rtype
