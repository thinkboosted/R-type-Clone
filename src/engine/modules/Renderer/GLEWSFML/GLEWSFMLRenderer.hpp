/**
 * @file GLEWSFMLRenderer.hpp
 * @brief OpenGL/GLEW-based 3D renderer module
 * 
 * @details Renders 3D scenes using OpenGL with GLEW for extension loading.
 * Manages render objects, cameras, lights, and particle systems.
 * 
 * @section channels_sub Subscribed Channels
 * | Channel | Payload | Description |
 * |---------|---------|-------------|
 * | `RenderEntityCommand` | Command string | Entity rendering commands |
 * | `WindowResized` | "width,height" | Handle window resize |
 * 
 * @section render_commands RenderEntityCommand Formats
 * - `CreateEntity:mesh:id` - Create entity with mesh
 * - `CreateEntity:mesh:texture:id` - Create entity with mesh and texture
 * - `SetPosition:id,x,y,z` - Set entity position
 * - `SetRotation:id,rx,ry,rz` - Set entity rotation
 * - `SetScale:id,sx,sy,sz` - Set entity scale
 * - `SetColor:id,r,g,b` - Set entity color
 * - `SetTexture:id:path` - Set entity texture
 * - `SetActiveCamera:id` - Set active camera
 * - `CreateParticleGenerator:params` - Create particle system
 * 
 * @section channels_pub Published Channels
 * | Channel | Payload | Description |
 * |---------|---------|-------------|
 * | `ImageRendered` | Raw pixel data | Frame ready for display |
 * 
 * @see docs/CHANNELS.md for complete channel reference
 */

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
