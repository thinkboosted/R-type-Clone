#pragma once

#include <GL/glew.h>
#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdint>
#include <memory>
#include <map>
#include <chrono>
#include "../I3DRenderer.hpp"
#include "RenderStructs.hpp"
#include "ResourceManager.hpp"
#include "ParticleSystem.hpp"

namespace rtypeEngine {
class GLEWSFMLRenderer : public I3DRenderer {
  public:
    explicit GLEWSFMLRenderer(const char* pubEndpoint, const char* subEndpoint);
    ~GLEWSFMLRenderer() override = default;

    void init() override;
    void loop() override;
    void cleanup() override;

    void onRenderEntityCommand(const std::string& message);
    void handleWindowResized(const std::string& message);

    void clearBuffer() override;
    void render() override;
    std::vector<uint32_t> getPixels() const override;
    Vector2u getResolution() const override;

  private:
    void createFramebuffer();
    void destroyFramebuffer();
    void ensureGLEWInitialized();
    void initContext();

    // Moved to ResourceManager
    // void loadMesh(const std::string& path);
    // GLuint loadTexture(const std::string& path);
    // GLuint createTextTexture(const std::string& text, const std::string& fontPath, unsigned int fontSize, Vector3f color);

    Vector2u _resolution;
    Vector2u _hudResolution;
    bool _pendingResize = false;
    Vector2u _newResolution;
    GLuint _framebuffer;
    GLuint _renderTexture;
    GLuint _depthBuffer;
    std::vector<uint32_t> _pixelBuffer;

    std::map<std::string, RenderObject> _renderObjects;
    std::chrono::steady_clock::time_point _lastFrameTime;

    std::string _activeCameraId;
    Vector3f _cameraPos;
    Vector3f _cameraRot;
    Vector3f _lightPos;
    Vector3f _lightColor;
    float _lightIntensity;
    std::string _activeLightId;
    bool _glewInitialized = false;
    void* _hwnd = nullptr;
    void* _hdc = nullptr;
    void* _hglrc = nullptr;

    ResourceManager _resourceManager;
    ParticleSystem _particleSystem;
};
}  // namespace rtypeEngine
