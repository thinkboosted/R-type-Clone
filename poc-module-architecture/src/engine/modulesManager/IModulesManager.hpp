
#pragma once

#include "IModule.hpp"
#include <memory>
#include <vector>
#include <string>

namespace rtypeEngine {
class IModulesManager {
  public:
    virtual ~IModulesManager() = default;
    virtual std::shared_ptr<IModule> loadModule(const std::string &modulePath, const std::string &pubEndpoint, const std::string &subEndpoint) = 0;

  protected:
    std::vector<std::shared_ptr<IModule>> _modules;
};
}  // namespace rtypeEngine
