#pragma once

#include <vector>
#include <cstdint>
#include "../AModule.hpp"
#include "../../types/vector.hpp"

namespace rtypeEngine {
class I3DRenderer : public AModule {
  public:
    explicit I3DRenderer(const char* pubEndpoint, const char* subEndpoint)
        : AModule(pubEndpoint, subEndpoint) {}
    virtual ~I3DRenderer() = default;

  protected:
    virtual void addMesh() = 0;
    virtual void clearBuffer() = 0;
    virtual void render() = 0;
    virtual std::vector<uint32_t> getPixels() const = 0;
    virtual Vector2u getResolution() const = 0;
};
}  // namespace rtypeEngine
