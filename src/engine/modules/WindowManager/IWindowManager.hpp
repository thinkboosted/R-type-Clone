#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "../AModule.hpp"
#include "../../types/vector.hpp"


namespace rtypeEngine {
class IWindowManager : public AModule {
  public:
    using AModule::AModule;
    virtual ~IWindowManager() = default;

    protected:
    virtual void createWindow(const std::string &title, const Vector2u &size) = 0;
    virtual bool isOpen() const = 0;
    virtual void close() = 0;
    virtual void drawPixels(const std::vector<uint32_t> &pixels, const Vector2u &size) = 0;
};
}  // namespace rtypeEngine
