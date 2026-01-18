/**
 * @file I3DRenderer.hpp
 * @brief Interface for 3D rendering modules
 * 
 * @details Defines the contract for 3D rendering implementations.
 * 
 * @see GLEWSFMLRenderer for OpenGL/GLEW implementation
 * @see docs/CHANNELS.md for channel reference
 */

#pragma once

#include <vector>
#include <cstdint>
#include "../AModule.hpp"
#include "../../types/vector.hpp"

namespace rtypeEngine {
class I3DRenderer : public AModule {
  public:
    using AModule::AModule;
    virtual ~I3DRenderer() = default;

  protected:
    virtual void clearBuffer() = 0;
    virtual void render() = 0;
    virtual std::vector<uint32_t> getPixels() const = 0;
    virtual Vector2u getResolution() const = 0;
};
}  // namespace rtypeEngine
