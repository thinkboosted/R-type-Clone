#include "GLEWRenderer.hpp"
#include <stdexcept>
#include <iostream>
#include <thread>
#include <chrono>

namespace rtypeEngine {

GLEWRenderer::GLEWRenderer(const char* pubEndpoint, const char* subEndpoint)
    : I3DRenderer(pubEndpoint, subEndpoint),
      _resolution{800, 600},
      _framebuffer(0),
      _renderTexture(0),
      _depthBuffer(0),
      _glewInitialized(false) {}

GLEWRenderer::~GLEWRenderer() {
}

void GLEWRenderer::init() {
}

void GLEWRenderer::ensureGLEWInitialized() {

}

void GLEWRenderer::loop() {

    // TODO REMOVE
    static std::vector<uint32_t> mockPixels(_resolution.x * _resolution.y);
    static int frame = 0;

    for (uint32_t y = 0; y < _resolution.y; y++) {
        for (uint32_t x = 0; x < _resolution.x; x++) {
            uint32_t index = y * _resolution.x + x;

            uint8_t r = static_cast<uint8_t>((x + frame) % 256);
            uint8_t g = static_cast<uint8_t>((y + frame / 2) % 256);
            uint8_t b = static_cast<uint8_t>((x + y + frame) % 256);
            uint8_t a = 255;

            mockPixels[index] = (a << 24) | (b << 16) | (g << 8) | r;
        }
    }

    frame++;
    std::string pixelData(reinterpret_cast<const char*>(mockPixels.data()),
                         mockPixels.size() * sizeof(uint32_t));
    // TODO END

    sendMessage("ImageRendered", pixelData);
}

void GLEWRenderer::cleanup() {
}

void GLEWRenderer::createFramebuffer() {
}

void GLEWRenderer::destroyFramebuffer() {
}

void GLEWRenderer::addMesh() {
}

void GLEWRenderer::clearBuffer() {
}

void GLEWRenderer::render() {
}

std::vector<uint32_t> GLEWRenderer::getPixels() const {
    return _pixelBuffer;
}

Vector2u GLEWRenderer::getResolution() const {
    return _resolution;
}

}  // namespace rtypeEngine

// Factory function for dynamic loading
extern "C" __declspec(dllexport) rtypeEngine::IModule* createModule(const char* pubEndpoint, const char* subEndpoint) {
    return new rtypeEngine::GLEWRenderer(pubEndpoint, subEndpoint);
}
