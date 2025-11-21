
#pragma once

#include "../engine/app/AApplication.hpp"
#include <string>

namespace rtypeGame {
class Rtype : public rtypeEngine::AApplication {
  public:
    Rtype(const std::string &endpoint);
    void init() override;
    void loop() override;

  private:
    bool _scriptsLoaded = false;
};
}  // namespace rtype
