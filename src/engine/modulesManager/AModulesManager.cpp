
#include "AModulesManager.hpp"
#include <stdexcept>

namespace rtypeEngine {

AModulesManager::~AModulesManager() {
    for (auto& module : _modules) {
        if (module) {
            module->stop();
        }
    }
    _modules.clear();

    for (auto handle : _handles) {
        if (handle) {
#ifdef _WIN32
            FreeLibrary(handle);
#else
            dlclose(handle);
#endif
        }
    }
    _handles.clear();
}

std::shared_ptr<IModule> AModulesManager::loadModule(const std::string &modulePath, const std::string &pubEndpoint, const std::string &subEndpoint) {
    ModuleHandle handle;
#ifdef _WIN32
    handle = LoadLibraryA(modulePath.c_str());
    if (!handle) {
        DWORD error = GetLastError();
        throw std::runtime_error("Failed to load module: " + modulePath + " (Error: " + std::to_string(error) + ")");
    }
#else
    handle = dlopen(modulePath.c_str(), RTLD_LAZY);
    if (!handle) {
        throw std::runtime_error("Failed to load module: " + std::string(dlerror()));
    }
#endif

    using CreateModuleFn = rtypeEngine::IModule*(*)(const char*, const char*);
    CreateModuleFn createModule;

#ifdef _WIN32
    createModule = reinterpret_cast<CreateModuleFn>(GetProcAddress(handle, "createModule"));
    if (!createModule) {
        FreeLibrary(handle);
        throw std::runtime_error("Failed to find createModule function in: " + modulePath);
    }
#else
    createModule = reinterpret_cast<CreateModuleFn>(dlsym(handle, "createModule"));
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        dlclose(handle);
        throw std::runtime_error("Failed to find createModule function: " + std::string(dlsym_error));
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

    return module;
}
}  // namespace rtypeEngine