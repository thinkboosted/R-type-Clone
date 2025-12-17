#include "AModulesManager.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
bool debugEnabled() {
    static bool enabled = (std::getenv("RTYPE_DEBUG") != nullptr);
    return enabled;
}
}

namespace rtypeEngine {
std::shared_ptr<IModule> AModulesManager::loadModule(const std::string &modulePath, const std::string &pubEndpoint, const std::string &subEndpoint) {
    ModuleHandle handle;
    #ifdef _WIN32
        handle = LoadLibrary(modulePath.c_str());
        if (!handle) {
            throw std::runtime_error("Cannot load module: " + modulePath);
        }
        createModule_t createModule = (createModule_t)GetProcAddress(handle, "createModule");
        if (!createModule) {
            FreeLibrary(handle);
            throw std::runtime_error("Cannot find createModule function in " + modulePath);
        }
    #else
        handle = dlopen(modulePath.c_str(), RTLD_LAZY);
        if (!handle) {
            throw std::runtime_error("Cannot load module: " + modulePath + " - " + dlerror());
        }
        createModule_t createModule = (createModule_t)dlsym(handle, "createModule");
        if (!createModule) {
            dlclose(handle);
            throw std::runtime_error("Cannot find createModule function in " + modulePath + " - " + dlerror());
        }
    #endif

    IModule* rawModule = createModule(pubEndpoint.c_str(), subEndpoint.c_str());

    if (!rawModule) {
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        throw std::runtime_error("createModule returned nullptr for: " + modulePath);
    }

    std::shared_ptr<IModule> module(rawModule);
    _modules.push_back(module);
    _handles.push_back(handle);

    if (debugEnabled()) {
        std::cout << "[ModulesManager] Loaded " << modulePath << " pub=" << pubEndpoint << " sub=" << subEndpoint << std::endl;
    }

    return module;
}

AModulesManager::~AModulesManager() {
    _modules.clear(); // Clear shared_ptrs before unloading libraries
    for (ModuleHandle handle : _handles) {
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
    }
}

}  // namespace rtypeEngine