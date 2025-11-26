#pragma once

#include <GL/glew.h>
#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdint>
#include <memory>
#include <map>
#include "../I3DRenderer.hpp"

namespace rtypeEngine {
class GLEWSFMLRenderer : public I3DRenderer {
  public:
    explicit GLEWSFMLRenderer(const char* pubEndpoint, const char* subEndpoint);
    ~GLEWSFMLRenderer() override;

    void init() override;
    void loop() override;
    void cleanup() override;

    void onRenderEntityCommand(const std::string& message);

    void addMesh(/* mesh parameters */) override;
    void clearBuffer() override;
    void render() override;
    std::vector<uint32_t> getPixels() const override;
    Vector2u getResolution() const override;

  private:
    void createFramebuffer();
    void destroyFramebuffer();
    void ensureGLEWInitialized();
    void initContext();
    void loadMesh(const std::string& path);
    GLuint loadTexture(const std::string& path);

    struct RenderObject {
        std::string id;
        std::string meshPath;
        std::string texturePath;
        bool isSprite = false;
        bool isScreenSpace = false;
        Vector3f position;
        Vector3f rotation;
        Vector3f scale;
        Vector3f color;
    };

    struct MeshData {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
    };

    Vector2u _resolution;
    GLuint _framebuffer;
    GLuint _renderTexture;
    GLuint _depthBuffer;
    std::vector<uint32_t> _pixelBuffer;

    std::map<std::string, RenderObject> _renderObjects;
    std::map<std::string, MeshData> _meshCache;
    std::map<std::string, GLuint> _textureCache;
    std::string _activeCameraId;
    Vector3f _cameraPos;
    Vector3f _cameraRot;
    Vector3f _lightPos;
    Vector3f _lightColor;
    float _lightIntensity;
    std::string _activeLightId;
    bool _glewInitialized = false;
#ifdef _WIN32
    void* _hwnd = nullptr;
    void* _hdc = nullptr;
    void* _hglrc = nullptr;
#endif
};
}  // namespace rtypeEngine
