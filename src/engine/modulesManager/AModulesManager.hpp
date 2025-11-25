
#pragma once

#include "IModulesManager.hpp"

#ifdef _WIN32
    #include <windows.h>
    using ModuleHandle = HMODULE;
#else
    #include <dlfcn.h>
    using ModuleHandle = void*;
#endif

namespace rtypeEngine {
class AModulesManager : public IModulesManager {
  public:
    ~AModulesManager() override;
    std::shared_ptr<IModule> loadModule(const std::string &modulePath, const std::string &pubEndpoint, const std::string &subEndpoint) override;

  protected:
    std::vector<ModuleHandle> _handles;
};
}  // namespace rtypeEngine
