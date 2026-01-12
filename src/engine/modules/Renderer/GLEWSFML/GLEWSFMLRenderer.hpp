#pragma once

#include <GL/glew.h>
#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdint>
#include <memory>
#include <map>
#include <chrono>
#include "../I3DRenderer.hpp"

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
    void updateParticles(float dt);
    void renderParticles();

    void createFramebuffer();
    void destroyFramebuffer();
    void ensureGLEWInitialized();
    void initContext();
    void loadMesh(const std::string& path);
    GLuint loadTexture(const std::string& path);
    GLuint createTextTexture(const std::string& text, const std::string& fontPath, unsigned int fontSize, Vector3f color);

    struct RenderObject {
        std::string id;
        std::string meshPath;
        std::string texturePath;
        bool isSprite = false;
        bool isScreenSpace = false;
        bool isText = false;
        bool isRect = false;
        bool isCircle = false;
        bool isRoundedRect = false;
        bool isLine = false;
        float cornerRadius = 0.0f;  // For rounded rectangles
        float radius = 0.0f;        // For circles
        int segments = 32;          // Circle/rounded corner segments
        float lineWidth = 1.0f;     // For lines
        Vector3f endPosition;       // End point for lines
        bool outlined = false;      // Draw outline only
        float outlineWidth = 2.0f;  // Outline thickness
        Vector3f outlineColor;      // Outline color
        std::string text;
        std::string fontPath;
        unsigned int fontSize = 24;
        Vector3f position;
        Vector3f rotation;
        Vector3f scale;
        Vector3f color;
        float alpha = 1.0f;
        int zOrder = 0;
        GLuint textureID = 0; // Custom texture ID for texts
    };

    struct Particle {
        Vector3f position;
        Vector3f velocity;
        Vector3f color;
        float life;
        float maxLife;
        float size;
    };

    struct ParticleGenerator {
        std::string id;
        Vector3f position;
        Vector3f rotation;
        Vector3f offset;
        Vector3f direction;
        float spread;
        float speed;
        float lifeTime;
        float rate;
        float size;
        Vector3f color;

        float accumulator = 0.0f;
        std::vector<Particle> particles;
    };

    struct MeshData {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        std::vector<float> uvs;
        std::vector<unsigned int> _textureIndices;
    };

    Vector2u _resolution;
    Vector2u _hudResolution;
    bool _pendingResize = false;
    Vector2u _newResolution;
    GLuint _framebuffer;
    GLuint _renderTexture;
    GLuint _depthBuffer;
    std::vector<uint32_t> _pixelBuffer;

    std::map<std::string, RenderObject> _renderObjects;
    std::map<std::string, ParticleGenerator> _particleGenerators;
    std::chrono::steady_clock::time_point _lastFrameTime;

    std::map<std::string, MeshData> _meshCache;
    std::map<std::string, GLuint> _textureCache;
    std::map<std::string, sf::Font> _fontCache;
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
};
}  // namespace rtypeEngine
