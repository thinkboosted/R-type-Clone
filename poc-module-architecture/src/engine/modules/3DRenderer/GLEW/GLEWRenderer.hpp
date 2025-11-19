#pragma once

#include <GL/glew.h>
#include <vector>
#include <cstdint>
#include <memory>
#include "../I3DRenderer.hpp"

namespace rtypeEngine {
class GLEWRenderer : public I3DRenderer {
  public:
    explicit GLEWRenderer(const char* pubEndpoint, const char* subEndpoint);
    ~GLEWRenderer() override;

    void init() override;
    void loop() override;
    void cleanup() override;

    void addMesh(/* mesh parameters */) override;
    void clearBuffer() override;
    void render() override;
    std::vector<uint32_t> getPixels() const override;
    Vector2u getResolution() const override;

  private:
    void createFramebuffer();
    void destroyFramebuffer();
    void ensureGLEWInitialized();
    void initialize();

    Vector2u _resolution;
    GLuint _framebuffer;
    GLuint _renderTexture;
    GLuint _depthBuffer;
    std::vector<uint32_t> _pixelBuffer;
    bool _glewInitialized;
};
}  // namespace rtypeEngine
