#ifdef _WIN32
#define GLEW_RENDERER_EXPORT __declspec(dllexport)
#else
#define GLEW_RENDERER_EXPORT
#endif

#include "GLEWSFMLRenderer.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#endif
#include <stdexcept>
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <fstream>
#include <cmath>
#include <random>
#include <cstdlib>
#include <cerrno>
#include <algorithm>

constexpr float PI = 3.14159265f;

namespace rtypeEngine
{

namespace {
[[nodiscard]] inline float safeParseFloat(const std::string& str, float fallback = 0.0f) noexcept {
    errno = 0;
    char* end = nullptr;
    const float value = std::strtof(str.c_str(), &end);
    if (end == str.c_str() || errno == ERANGE) return fallback;
    return value;
}

[[nodiscard]] inline int safeParseInt(const std::string& str, int fallback = 0) noexcept {
    errno = 0;
    char* end = nullptr;
    const long value = std::strtol(str.c_str(), &end, 10);
    if (end == str.c_str() || errno == ERANGE) return fallback;
    return static_cast<int>(value);
}

template<typename T>
[[nodiscard]] inline constexpr T clamp(T val, T min, T max) noexcept {
    return val < min ? min : (val > max ? max : val);
}
} // namespace

    GLEWSFMLRenderer::GLEWSFMLRenderer(const char *pubEndpoint, const char *subEndpoint)
        : I3DRenderer(pubEndpoint, subEndpoint),
          _resolution{800, 600},
          _hudResolution{800, 600},
          _framebuffer(0),
          _renderTexture(0),
          _depthBuffer(0),
          _glewInitialized(false),
          _hwnd(nullptr),
          _hdc(nullptr),
          _hglrc(nullptr),
          _cameraPos{0.0f, 0.0f, 5.0f},
          _cameraRot{0.0f, 0.0f, 0.0f},
          _lightPos{0.0f, 5.0f, 0.0f},
          _lightColor{1.0f, 1.0f, 1.0f},
          _lightIntensity(1.0f),
          _lastFrameTime(std::chrono::steady_clock::now())
    {
        _pixelBuffer.resize(_resolution.x * _resolution.y);
    }

    GLEWSFMLRenderer::~GLEWSFMLRenderer()
    {
        GLEWSFMLRenderer::cleanup();
    }

    void GLEWSFMLRenderer::init()
    {
        subscribe("", [this](const auto& msg) {
            if (msg.find("FrameMetrics") == std::string::npos)
                std::cout << "[GLEW-ALL] " << msg << std::endl;
        });

        subscribe("RenderEntityCommand", [this](const auto& msg) { onRenderEntityCommand(msg); });
        subscribe("CreateText", [this](const auto& msg) { loadText(msg); });
        subscribe("WindowResized", [this](const auto& msg) { handleWindowResized(msg); });

        initContext();
        ensureGLEWInitialized();
        createFramebuffer();
        glEnable(GL_DEPTH_TEST);

        std::cout << "[GLEWSFMLRenderer] Initialized" << std::endl;
    }

    void GLEWSFMLRenderer::ensureGLEWInitialized()
    {
        if (_glewInitialized) return;

        glewExperimental = GL_TRUE;
        if (const auto err = glewInit(); GLEW_OK != err) {
            std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
            return;
        }
        _glewInitialized = true;
        glGetError(); // Clear any error from glewInit
    }

    void GLEWSFMLRenderer::loop()
    {
#ifdef _WIN32
        wglMakeCurrent((HDC)_hdc, (HGLRC)_hglrc);
#else
        glXMakeCurrent((Display *)_hdc, (Window)_hwnd, (GLXContext)_hglrc);
#endif
        render();
        // Optimization: Do NOT release context here. Keep it active for the thread.
        // Releasing it causes unnecessary switches and potential conflicts if other functions try to restore it.
        /*
#ifdef _WIN32
        wglMakeCurrent(NULL, NULL); // Release context
#else
        glXMakeCurrent(NULL, None, NULL); // Release context
#endif
        */
    }

    void GLEWSFMLRenderer::loadText(const std::string &message)
    {
        std::cout << "[GLEWSFMLRenderer] loadText: " << message << std::endl;
        std::stringstream ss(message);
        std::string id, text, fontPath, sizeStr, screenStr;

        if (std::getline(ss, id, ';') &&
            std::getline(ss, text, ';') &&
            std::getline(ss, fontPath, ';') &&
            std::getline(ss, sizeStr, ';') &&
            std::getline(ss, screenStr)) // Last one doesn't need delimiter if it's the end
        {
            try {
                RenderObject obj;
                obj.id = id;
                obj.isText = true;
                obj.isSprite = true;
                obj.text = text;
                obj.fontPath = fontPath;
                obj.fontSize = safeParseInt(sizeStr, 24);
                obj.isScreenSpace = (screenStr == "1");
                obj.position = {0, 0, 0};
                obj.rotation = {0, 0, 0};
                obj.scale = {1, 1, 1};
                obj.color = {1.0f, 1.0f, 1.0f}; // Default white

                obj.textureID = createTextTexture(obj.text, obj.fontPath, obj.fontSize, obj.color);

                if (obj.textureID != 0) {
                    _renderObjects[id] = obj;
                    std::cout << "[GLEWSFMLRenderer] Created Text Object ID: " << id << " TexID: " << obj.textureID << std::endl;
                } else {
                    std::cerr << "[GLEWSFMLRenderer] Failed to create text texture for ID: " << id << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "[GLEWSFMLRenderer] Error in loadText: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "[GLEWSFMLRenderer] Invalid CreateText message format: " << message << std::endl;
        }
    }

    void GLEWSFMLRenderer::handleWindowResized(const std::string &message)
    {
        std::stringstream ss(message);
        std::string widthStr, heightStr;

        if (std::getline(ss, widthStr, ',') && std::getline(ss, heightStr))
        {
            try
            {
                unsigned int width = std::stoul(widthStr);
                unsigned int height = std::stoul(heightStr);

                if (width > 0 && height > 0)
                {
                    _newResolution = {width, height};
                    _pendingResize = true;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "[GLEWSFMLRenderer] Error parsing resize message: " << e.what() << std::endl;
            }
        }
    }

    void GLEWSFMLRenderer::onRenderEntityCommand(const std::string &message)
    {
        // std::cout << "[GLEW] CMD RAW: " << message << std::endl; // Too verbose for position updates
        std::stringstream ss(message);
        std::string segment;
        while (std::getline(ss, segment, ';'))
        {
            if (segment.empty())
                continue;

            size_t colonPos = segment.find(':');
            if (colonPos == std::string::npos)
                continue;

            std::string command = segment.substr(0, colonPos);
            std::string data = segment.substr(colonPos + 1);

            if (command == "CreateText") {
                 std::cout << "[GLEW] Received CreateText: " << data << std::endl;
            }

        try {
        if (command == "CreateEntity") {
             size_t split = data.find(':');
             if (split != std::string::npos) {
                 std::string type = data.substr(0, split);
                 std::string id = data.substr(split + 1);

                    if (type == "Camera" || type == "CAMERA")
                    {
                        RenderObject obj;
                        obj.id = id;
                        obj.meshPath = "";
                        obj.position = {0, 0, 0};
                        obj.rotation = {0, 0, 0};
                        obj.scale = {1, 1, 1};
                        obj.color = {1.0f, 1.0f, 1.0f};
                        _renderObjects[id] = obj;
                    }
                    else if (type == "Light" || type == "LIGHT")
                    {
                        _activeLightId = id;
                    }
                    else if (type == "Sprite" || type == "SPRITE")
                    {
                        size_t split2 = id.find(':');
                        if (split2 != std::string::npos)
                        {
                            std::string texturePath = id.substr(0, split2);
                            std::string realId = id.substr(split2 + 1);

                            RenderObject obj;
                            obj.id = realId;
                            obj.isSprite = true;
                            obj.texturePath = texturePath;
                            obj.position = {0, 0, 0};
                            obj.rotation = {0, 0, 0};
                            obj.scale = {1, 1, 1};
                            obj.color = {1.0f, 1.0f, 1.0f};
                            _renderObjects[realId] = obj;
                        }
                    }
                    else if (type == "HUDSprite" || type == "HUDSPRITE")
                    {
                        size_t split2 = id.find(':');
                        if (split2 != std::string::npos)
                        {
                            std::string texturePath = id.substr(0, split2);
                            std::string realId = id.substr(split2 + 1);

                            RenderObject obj;
                            obj.id = realId;
                            obj.isSprite = true;
                            obj.isScreenSpace = true;
                            obj.texturePath = texturePath;
                            obj.position = {0, 0, 0};
                            obj.rotation = {0, 0, 0};
                            obj.scale = {1, 1, 1};
                            obj.color = {1.0f, 1.0f, 1.0f};
                            _renderObjects[realId] = obj;
                        }
                    }
                    else if (type == "MESH")
                    {
                        // data format: MESH:path:id:0:0:0:0
                        std::stringstream mss(id);
                        std::string path, realId;
                        if (std::getline(mss, path, ':') && std::getline(mss, realId, ':')) {
                            RenderObject obj;
                            obj.id = realId;
                            obj.meshPath = path;
                            obj.position = {0, 0, 0};
                            obj.rotation = {0, 0, 0};
                            obj.scale = {1, 1, 1};
                            obj.color = {1.0f, 1.0f, 1.0f};
                            _renderObjects[realId] = obj;

                            if (_meshCache.find(obj.meshPath) == _meshCache.end())
                            {
                                loadMesh(obj.meshPath);
                            }
                        }
                    }
                    else
                    {
                        RenderObject obj;
                        obj.id = id;
                        if (type == "cube")
                            obj.meshPath = "assets/models/cube.obj";
                        else
                            obj.meshPath = type;

                        obj.position = {0, 0, 0};
                        obj.rotation = {0, 0, 0};
                        obj.scale = {1, 1, 1};
                        obj.color = {1.0f, 0.5f, 0.2f};
                        _renderObjects[id] = obj;

                        if (!_meshCache.count(obj.meshPath))
                        {
                            loadMesh(obj.meshPath);
                        }
                    }
                }
            }
            else if (command == "CreateText")
            {
                size_t split = data.find(':');
                if (split != std::string::npos)
                {
                    std::string id = data.substr(0, split);
                    std::string rest = data.substr(split + 1);

                    std::stringstream ss2(rest);
                    std::string textContent, fontPath, fontSizeStr, isScreenSpaceStr;
                    std::getline(ss2, fontPath, ':');
                    std::getline(ss2, fontSizeStr, ':');
                    std::getline(ss2, isScreenSpaceStr, ':');
                    std::getline(ss2, textContent);

                    try
                    {
                        RenderObject obj;
                        obj.id = id;
                        obj.isText = true;
                        obj.isSprite = true;
                        obj.text = textContent;
                        obj.fontPath = fontPath;
                        obj.fontSize = safeParseInt(fontSizeStr, 24);
                        obj.isScreenSpace = (isScreenSpaceStr == "1" || isScreenSpaceStr == "true");
                        obj.position = {0, 0, 0};
                        obj.rotation = {0, 0, 0};
                        obj.scale = {1, 1, 1};
                        obj.color = {1.0f, 1.0f, 1.0f};

                        obj.textureID = createTextTexture(obj.text, obj.fontPath, obj.fontSize, obj.color);
                        _renderObjects[id] = obj;
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "[GLEWSFMLRenderer] Error creating text: " << e.what() << std::endl;
                    }
                }
            }
            else if (command == "CreateRect")
            {
                // Format: CreateRect:id:x,y,width,height:r,g,b,a:isScreenSpace
                size_t split = data.find(':');
                if (split != std::string::npos)
                {
                    std::string id = data.substr(0, split);
                    std::string rest = data.substr(split + 1);

                    std::stringstream ss2(rest);
                    std::string posSize, colorStr, isScreenSpaceStr;
                    std::getline(ss2, posSize, ':');
                    std::getline(ss2, colorStr, ':');
                    std::getline(ss2, isScreenSpaceStr);

                    std::stringstream pss(posSize);
                    std::string val;
                    std::vector<float> pos;
                    while(std::getline(pss, val, ',')) pos.push_back(safeParseFloat(val));

                    std::stringstream css(colorStr);
                    std::vector<float> col;
                    while(std::getline(css, val, ',')) col.push_back(safeParseFloat(val));

                    if (pos.size() >= 4 && col.size() >= 3)
                    {
                        RenderObject obj;
                        obj.id = id;
                        obj.isRect = true;
                        obj.isScreenSpace = (isScreenSpaceStr == "1" || isScreenSpaceStr == "true");
                        obj.position = {pos[0], pos[1], 0};
                        // Validate dimensions
                        obj.scale = {std::max(0.0f, pos[2]), std::max(0.0f, pos[3]), 1};  // width, height stored in scale
                        obj.color = {col[0], col[1], col[2]};
                        float alphaRaw = (col.size() >= 4) ? col[3] : 1.0f;
                        obj.alpha = std::max(0.0f, std::min(1.0f, alphaRaw));
                        _renderObjects[id] = obj;
                    }
                }
            }
            else if (command == "SetRect")
            {
                // Format: SetRect:id:x,y,width,height
                size_t split = data.find(':');
                if (split != std::string::npos)
                {
                    std::string id = data.substr(0, split);
                    std::string params = data.substr(split + 1);

                    std::stringstream pss(params);
                    std::string val;
                    std::vector<float> v;
                    while(std::getline(pss, val, ',')) v.push_back(safeParseFloat(val));

                    if (v.size() >= 4 && _renderObjects.find(id) != _renderObjects.end())
                    {
                        auto& obj = _renderObjects[id];
                        obj.position = {v[0], v[1], 0};
                        obj.scale = {std::max(0.0f, v[2]), std::max(0.0f, v[3]), 1};
                    }
                }
            }
            else if (command == "SetAlpha")
            {
                // Format: SetAlpha:id:alpha
                size_t split = data.find(':');
                if (split != std::string::npos)
                {
                    std::string id = data.substr(0, split);
                    float alpha = safeParseFloat(data.substr(split + 1), 1.0f);

                    if (_renderObjects.find(id) != _renderObjects.end())
                    {
                        // Clamp alpha between 0.0 and 1.0
                        _renderObjects[id].alpha = std::max(0.0f, std::min(1.0f, alpha));
                    }
                }
            }
            else if (command == "SetZOrder")
            {
                // Format: SetZOrder:id:zorder
                size_t split = data.find(':');
                if (split != std::string::npos)
                {
                    std::string id = data.substr(0, split);
                    int zorder = safeParseInt(data.substr(split + 1), 0);

                    if (_renderObjects.find(id) != _renderObjects.end())
                    {
                        _renderObjects[id].zOrder = zorder;
                    }
                }
            }
            else if (command == "CreateCircle")
            {
                // Format: CreateCircle:id:x,y,radius:r,g,b,a:isScreenSpace:segments
                size_t split = data.find(':');
                if (split != std::string::npos)
                {
                    std::string id = data.substr(0, split);
                    std::string rest = data.substr(split + 1);

                    std::stringstream ss2(rest);
                    std::string posRadius, colorStr, isScreenSpaceStr, segmentsStr;
                    std::getline(ss2, posRadius, ':');
                    std::getline(ss2, colorStr, ':');
                    std::getline(ss2, isScreenSpaceStr, ':');
                    std::getline(ss2, segmentsStr);

                    std::stringstream pss(posRadius);
                    std::string val;
                    std::vector<float> pos;
                    while(std::getline(pss, val, ',')) pos.push_back(safeParseFloat(val));

                    std::stringstream css(colorStr);
                    std::vector<float> col;
                    while(std::getline(css, val, ',')) col.push_back(safeParseFloat(val));

                    if (pos.size() >= 3 && col.size() >= 3)
                    {
                        RenderObject obj;
                        obj.id = id;
                        obj.isCircle = true;
                        obj.isScreenSpace = (isScreenSpaceStr == "1" || isScreenSpaceStr == "true");
                        obj.position = {pos[0], pos[1], 0};
                        obj.radius = std::max(0.0f, pos[2]);
                        obj.color = {col[0], col[1], col[2]};
                        obj.alpha = (col.size() >= 4) ? col[3] : 1.0f;
                        int segments = segmentsStr.empty() ? 32 : safeParseInt(segmentsStr, 32);
                        if (segments < 3) segments = 3;
                        else if (segments > 128) segments = 128;
                        obj.segments = segments;
                        _renderObjects[id] = obj;
                    }
                }
            }
            else if (command == "CreateRoundedRect")
            {
                // Format: CreateRoundedRect:id:x,y,width,height,cornerRadius:r,g,b,a:isScreenSpace
                size_t split = data.find(':');
                if (split != std::string::npos)
                {
                    std::string id = data.substr(0, split);
                    std::string rest = data.substr(split + 1);

                    std::stringstream ss2(rest);
                    std::string posSize, colorStr, isScreenSpaceStr;
                    std::getline(ss2, posSize, ':');
                    std::getline(ss2, colorStr, ':');
                    std::getline(ss2, isScreenSpaceStr);

                    std::stringstream pss(posSize);
                    std::string val;
                    std::vector<float> pos;
                    while(std::getline(pss, val, ',')) pos.push_back(safeParseFloat(val));

                    std::stringstream css(colorStr);
                    std::vector<float> col;
                    while(std::getline(css, val, ',')) col.push_back(safeParseFloat(val));

                    if (pos.size() >= 5 && col.size() >= 3)
                    {
                        RenderObject obj;
                        obj.id = id;
                        obj.isRoundedRect = true;
                        obj.isScreenSpace = (isScreenSpaceStr == "1" || isScreenSpaceStr == "true");
                        obj.position = {pos[0], pos[1], 0};
                        float w = std::max(0.0f, pos[2]);
                        float h = std::max(0.0f, pos[3]);
                        obj.scale = {w, h, 1};  // width, height
                        // Clamp corner radius to half of smallest dimension
                        float maxRadius = std::min(w, h) / 2.0f;
                        obj.cornerRadius = std::max(0.0f, std::min(pos[4], maxRadius));
                        obj.color = {col[0], col[1], col[2]};
                        float alphaRaw = (col.size() >= 4) ? col[3] : 1.0f;
                        obj.alpha = std::max(0.0f, std::min(1.0f, alphaRaw));
                        _renderObjects[id] = obj;
                    }
                }
            }
            else if (command == "CreateLine")
            {
                // Format: CreateLine:id:x1,y1,x2,y2,width:r,g,b,a:isScreenSpace
                size_t split = data.find(':');
                if (split != std::string::npos)
                {
                    std::string id = data.substr(0, split);
                    std::string rest = data.substr(split + 1);

                    std::stringstream ss2(rest);
                    std::string posStr, colorStr, isScreenSpaceStr;
                    std::getline(ss2, posStr, ':');
                    std::getline(ss2, colorStr, ':');
                    std::getline(ss2, isScreenSpaceStr);

                    std::stringstream pss(posStr);
                    std::string val;
                    std::vector<float> pos;
                    while(std::getline(pss, val, ',')) pos.push_back(safeParseFloat(val));

                    std::stringstream css(colorStr);
                    std::vector<float> col;
                    while(std::getline(css, val, ',')) col.push_back(safeParseFloat(val));

                    if (pos.size() >= 5 && col.size() >= 3)
                    {
                        RenderObject obj;
                        obj.id = id;
                        obj.isLine = true;
                        obj.isScreenSpace = (isScreenSpaceStr == "1" || isScreenSpaceStr == "true");
                        obj.position = {pos[0], pos[1], 0};
                        obj.endPosition = {pos[2], pos[3], 0};
                        obj.lineWidth = std::max(1.0f, std::min(pos[4], 50.0f));
                        obj.color = {col[0], col[1], col[2]};
                        float alphaRaw = (col.size() >= 4) ? col[3] : 1.0f;
                        obj.alpha = std::max(0.0f, std::min(1.0f, alphaRaw));
                        _renderObjects[id] = obj;
                    }
                }
            }
            else if (command == "CreateUISprite")
            {
                // Format: CreateUISprite:id:texturePath:x,y,width,height:isScreenSpace:zOrder
                size_t split = data.find(':');
                if (split != std::string::npos)
                {
                    std::string id = data.substr(0, split);
                    std::string rest = data.substr(split + 1);

                    std::stringstream ss2(rest);
                    std::string texturePath, posSize, isScreenSpaceStr, zOrderStr;
                    std::getline(ss2, texturePath, ':');
                    std::getline(ss2, posSize, ':');
                    std::getline(ss2, isScreenSpaceStr, ':');
                    std::getline(ss2, zOrderStr);

                    std::stringstream pss(posSize);
                    std::string val;
                    std::vector<float> pos;
                    while(std::getline(pss, val, ',')) pos.push_back(safeParseFloat(val));

                    if (pos.size() >= 4)
                    {
                        RenderObject obj;
                        obj.id = id;
                        obj.isSprite = true;
                        obj.isScreenSpace = (isScreenSpaceStr == "1" || isScreenSpaceStr == "true");
                        obj.texturePath = texturePath;
                        obj.position = {pos[0], pos[1], 0};
                        obj.scale = {pos[2], pos[3], 1};  // Store explicit width/height
                        obj.color = {1.0f, 1.0f, 1.0f};
                        obj.alpha = 1.0f;
                        obj.zOrder = zOrderStr.empty() ? 0 : safeParseInt(zOrderStr, 0);
                        _renderObjects[id] = obj;
                    }
                }
            }
            else if (command == "SetOutline")
            {
                // Format: SetOutline:id:enabled:width:r,g,b
                size_t split = data.find(':');
                if (split != std::string::npos)
                {
                    std::string id = data.substr(0, split);
                    std::string rest = data.substr(split + 1);

                    std::stringstream ss2(rest);
                    std::string enabledStr, widthStr, colorStr;
                    std::getline(ss2, enabledStr, ':');
                    std::getline(ss2, widthStr, ':');
                    std::getline(ss2, colorStr);

                    if (_renderObjects.find(id) != _renderObjects.end())
                    {
                        auto& obj = _renderObjects[id];
                        obj.outlined = (enabledStr == "1" || enabledStr == "true");
                        float rawWidth = safeParseFloat(widthStr, 2.0f);
                        obj.outlineWidth = std::max(1.0f, std::min(rawWidth, 50.0f));

                        std::stringstream css(colorStr);
                        std::string val;
                        std::vector<float> col;
                        while(std::getline(css, val, ',')) col.push_back(safeParseFloat(val));
                        if (col.size() >= 3)
                        {
                            obj.outlineColor = {col[0], col[1], col[2]};
                        }
                    }
                }
            }
            else if (command == "SetRadius")
            {
                // Format: SetRadius:id:radius
                size_t split = data.find(':');
                if (split != std::string::npos)
                {
                    std::string id = data.substr(0, split);
                    float radius = safeParseFloat(data.substr(split + 1), 10.0f);

                    if (_renderObjects.find(id) != _renderObjects.end())
                    {
                        _renderObjects[id].radius = radius;
                    }
                }
            }
            else if (command == "SetCornerRadius")
            {
                // Format: SetCornerRadius:id:radius
                size_t split = data.find(':');
                if (split != std::string::npos)
                {
                    std::string id = data.substr(0, split);
                    float radius = safeParseFloat(data.substr(split + 1), 5.0f);

                    if (_renderObjects.find(id) != _renderObjects.end())
                    {
                        auto& obj = _renderObjects[id];
                        // Clamp corner radius to half of smallest dimension
                        float maxRadius = std::min(obj.scale.x, obj.scale.y) / 2.0f;
                        obj.cornerRadius = std::min(radius, maxRadius);
                    }
                }
            } else if (command == "BatchDrawDebugLines") {
                 // Format: x1,y1,z1,x2,y2,z2,r,g,b;...
                 std::stringstream ss(data);
                 std::string lineSegment;
                 
                 while(std::getline(ss, lineSegment, ';')) {
                     if (lineSegment.empty()) continue;
                     
                     // Optimization: Fast parse without creating many vector/string objects
                     float v[9];
                     int idx = 0;
                     size_t start = 0;
                     size_t end = lineSegment.find(',');
                     
                     while (end != std::string::npos && idx < 9) {
                         // Use safeParseFloat to avoid exceptions and segfaults
                         v[idx++] = safeParseFloat(lineSegment.substr(start, end - start));
                         start = end + 1;
                         end = lineSegment.find(',', start);
                     }
                     // Last element
                     if (idx < 9) {
                         v[idx++] = safeParseFloat(lineSegment.substr(start));
                     }

                     if (idx == 9) {
                        DebugLine line;
                        line.start = {v[0], v[1], v[2]};
                        line.end = {v[3], v[4], v[5]};
                        line.color = {v[6], v[7], v[8]};
                        _debugLines.push_back(line);
                     }
                 }
            } else if (command == "DrawDebugLine")
            {
                // Format: DrawDebugLine:x1,y1,z1:x2,y2,z2:r,g,b
                size_t split1 = data.find(':');
                if (split1 != std::string::npos) {
                    std::string p1Str = data.substr(0, split1);
                    std::string rest = data.substr(split1 + 1);
                    size_t split2 = rest.find(':');
                    if (split2 != std::string::npos) {
                        std::string p2Str = rest.substr(0, split2);
                        std::string colorStr = rest.substr(split2 + 1);

                        std::vector<float> p1, p2, col;
                        std::stringstream ss1(p1Str); std::string v;
                        while(std::getline(ss1, v, ',')) p1.push_back(safeParseFloat(v));
                        std::stringstream ss2(p2Str);
                        while(std::getline(ss2, v, ',')) p2.push_back(safeParseFloat(v));
                        std::stringstream ssc(colorStr);
                        while(std::getline(ssc, v, ',')) col.push_back(safeParseFloat(v));

                        if (p1.size() == 3 && p2.size() == 3 && col.size() == 3) {
                            DebugLine line;
                            line.start = {p1[0], p1[1], p1[2]};
                            line.end = {p2[0], p2[1], p2[2]};
                            line.color = {col[0], col[1], col[2]};
                            _debugLines.push_back(line);
                        }
                    }
                }
                
            } else if (command == "DrawDebugText") {
                // Format: DrawDebugText:x,y,z:text:r,g,b
                size_t split1 = data.find(':');
                if (split1 != std::string::npos) {
                    std::string posStr = data.substr(0, split1);
                    std::string rest = data.substr(split1 + 1);
                    size_t split2 = rest.find(':');
                    if (split2 != std::string::npos) {
                        std::string text = rest.substr(0, split2);
                        std::string colorStr = rest.substr(split2 + 1);

                        std::vector<float> pos, col;
                        std::stringstream ss1(posStr); std::string v;
                        while(std::getline(ss1, v, ',')) pos.push_back(safeParseFloat(v));
                        std::stringstream ssc(colorStr);
                        while(std::getline(ssc, v, ',')) col.push_back(safeParseFloat(v));

                        if (pos.size() == 3 && col.size() == 3) {
                            DebugText dt;
                            dt.position = {pos[0], pos[1], pos[2]};
                            dt.text = text;
                            dt.color = {col[0], col[1], col[2]};
                            _debugTexts.push_back(dt);
                        }
                    }
                }
            }
            else if (command == "SetText")
            {
                size_t split = data.find(':');
                if (split != std::string::npos)
                {
                    std::string id = data.substr(0, split);
                    std::string newText = data.substr(split + 1);

                if (_renderObjects.find(id) != _renderObjects.end()) {
                    auto& obj = _renderObjects[id];
                    if (obj.isText) {
                        obj.text = newText;
                        if (obj.textureID) glDeleteTextures(1, &obj.textureID);
                        // Always create white texture so glColor3f in render() can tint it correctly
                        obj.textureID = createTextTexture(obj.text, obj.fontPath, obj.fontSize, {1.0f, 1.0f, 1.0f});
                    }
                }
            }
        }
        else if (command == "SetPosition")
        {
            std::stringstream dss(data);
            std::string id, val;
            std::getline(dss, id, ',');
            std::vector<float> v;
            while(std::getline(dss, val, ',')) v.push_back(safeParseFloat(val));

                if (v.size() >= 3)
                {
                    bool handled = false;
                    if (_renderObjects.find(id) != _renderObjects.end())
                    {
                        _renderObjects[id].position = {v[0], v[1], v[2]};
                        handled = true;
                    }
                    if (_particleGenerators.find(id) != _particleGenerators.end())
                    {
                        _particleGenerators[id].position = {v[0], v[1], v[2]};
                        handled = true;
                    }

                if (!handled) {
                    if (id == _activeCameraId) {
                        _cameraPos = {v[0], v[1], v[2]};
                    } else if (id == _activeLightId) {
                        _lightPos = {v[0], v[1], v[2]};
                    }
                }
            }
        } else if (command == "SetRotation") {
            std::stringstream dss(data);
            std::string id, val;
            std::getline(dss, id, ',');
            std::vector<float> v;
            while(std::getline(dss, val, ',')) v.push_back(safeParseFloat(val));

                if (v.size() >= 3)
                {
                    bool handled = false;
                    if (_renderObjects.find(id) != _renderObjects.end())
                    {
                        _renderObjects[id].rotation = {v[0], v[1], v[2]};
                        handled = true;
                    }
                    if (_particleGenerators.find(id) != _particleGenerators.end())
                    {
                        _particleGenerators[id].rotation = {v[0], v[1], v[2]};
                        handled = true;
                    }

                if (!handled) {
                    if (id == _activeCameraId) {
                        _cameraRot = {v[0], v[1], v[2]};
                    }
                }
            }
        } else if (command == "SetScale") {
            std::stringstream dss(data);
            std::string id, val;
            std::getline(dss, id, ',');
            std::vector<float> v;
            while(std::getline(dss, val, ',')) v.push_back(safeParseFloat(val));

            if (v.size() >= 3 && _renderObjects.find(id) != _renderObjects.end()) {
                _renderObjects[id].scale = {v[0], v[1], v[2]};
            }
        } else if (command == "SetColor") {
            std::stringstream dss(data);
            std::string id, val;
            std::getline(dss, id, ',');
            std::vector<float> v;
            while(std::getline(dss, val, ',')) v.push_back(safeParseFloat(val));

                if (v.size() >= 3 && _renderObjects.find(id) != _renderObjects.end())
                {
                    _renderObjects[id].color = {v[0], v[1], v[2]};
                }
            }
            else if (command == "SetTexture")
            {
                size_t split = data.find(':');
                if (split != std::string::npos)
                {
                    std::string id = data.substr(0, split);
                    std::string texturePath = data.substr(split + 1);

                    if (_renderObjects.find(id) != _renderObjects.end())
                    {
                        _renderObjects[id].texturePath = texturePath;
                        // loadTexture(texturePath);
                    }
                }
            }
            else if (command == "SetLightProperties")
            {
                std::stringstream dss(data);
                std::string id, val;
                std::getline(dss, id, ',');
                std::vector<float> v;
                while (std::getline(dss, val, ','))
                    v.push_back(safeParseFloat(val));

                if (v.size() >= 4)
                {
                    _lightColor = {v[0], v[1], v[2]};
                    _lightIntensity = v[3];
                }
            }
            else if (command == "SetActiveCamera")
            {
                _activeCameraId = data;
            }
            else if (command == "DestroyEntity")
            {
                if (_renderObjects.find(data) != _renderObjects.end())
                {
                    _renderObjects.erase(data);
                }
                if (_particleGenerators.find(data) != _particleGenerators.end())
                {
                    _particleGenerators.erase(data);
                }
            }
            else if (command == "CreateParticleGenerator")
            {
                // Format: ID:offsetX,offsetY,offsetZ:dirX,dirY,dirZ:spread:speed:life:rate:size:r,g,b
                // The comment implies colons, but the code parses commas. Let's support colon delimiters too by replacing them.
                size_t split = data.find(':');
                if (split != std::string::npos)
                {
                    std::string id = data.substr(0, split);
                    std::string params = data.substr(split + 1);

                    // Replace all colons in params with commas to handle grouped format
                    std::replace(params.begin(), params.end(), ':', ',');

                    ParticleGenerator gen;
                    gen.id = id;
                    gen.position = {0, 0, 0};
                    gen.rotation = {0, 0, 0};
                    gen.offset = {0, 0, 0};
                    gen.direction = {0, 1, 0};

                    std::stringstream pss(params);
                    std::string val;
                    std::vector<float> v;
                    while(std::getline(pss, val, ',')) v.push_back(safeParseFloat(val));

                    if (v.size() >= 14)
                    {
                        gen.offset = {v[0], v[1], v[2]};
                        gen.direction = {v[3], v[4], v[5]};
                        gen.spread = v[6];
                        gen.speed = v[7];
                        gen.lifeTime = v[8];
                        gen.rate = v[9];
                        gen.size = v[10];
                        gen.color = {v[11], v[12], v[13]};
                    }
                    else
                    {
                         // If we didn't get enough values, maybe it's the simpler format?
                         // Fallback defaults?
                    }
                    _particleGenerators[id] = gen;
                }
            }
            else if (command == "UpdateParticleGenerator")
            {
                // ID:x,y,z,dirX,dirY,dirZ,spread,speed,life,rate,size,r,g,b
                size_t split = data.find(':');
                if (split != std::string::npos)
                {
                    std::string id = data.substr(0, split);
                    std::string params = data.substr(split + 1);

                    if (_particleGenerators.find(id) != _particleGenerators.end())
                    {
                        auto& gen = _particleGenerators[id];
                        std::stringstream pss(params);
                        std::string val;
                        std::vector<float> v;
                        while(std::getline(pss, val, ',')) v.push_back(safeParseFloat(val));

                        // Expected 14 values
                        if (v.size() >= 14) {
                            gen.position = {v[0], v[1], v[2]};
                            gen.direction = {v[3], v[4], v[5]};
                            gen.spread = v[6];
                            gen.speed = v[7];
                            gen.lifeTime = v[8];
                            gen.rate = v[9];
                            gen.size = v[10];
                            gen.color = {v[11], v[12], v[13]};
                        }
                    }
                }
            }
    } catch (const std::exception& e) {
        std::cerr << "[GLEWSFMLRenderer] RenderEntity parse error: " << e.what() << " in msg='" << message << "'" << std::endl;
    }
}
}

    void GLEWSFMLRenderer::loadMesh(const std::string &path)
    {
        if (path.empty())
            return;
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::cerr << "Failed to open mesh file: " << path << std::endl;
            return;
        }

        MeshData meshData;
        std::string line;
        std::vector<Vector3f> tempVertices;
        std::vector<Vector2f> tempTextures;

        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::string prefix;
            ss >> prefix;

            if (prefix == "v")
            {
                Vector3f v;
                ss >> v.x >> v.y >> v.z;
                tempVertices.push_back(v);
            }
            else if (prefix == "f")
            {
                std::string vertexStr;
                std::vector<unsigned int> faceIndices;
                std::vector<unsigned int> faceUVIndices;
                while (ss >> vertexStr)
                {
                    size_t slashPos = vertexStr.find('/');
                    size_t secondSlashPos = vertexStr.find('/', slashPos + 1);

                    std::string indexStr = (slashPos != std::string::npos) ? vertexStr.substr(0, slashPos) : vertexStr;
                    std::string uvIndexStr;

                    if (slashPos != std::string::npos)
                    {
                        if (secondSlashPos != std::string::npos)
                        {
                            uvIndexStr = vertexStr.substr(slashPos + 1, secondSlashPos - slashPos - 1);
                        }
                        else
                        {
                            uvIndexStr = vertexStr.substr(slashPos + 1);
                        }
                    }

                    if (!indexStr.empty())
                    {
                        try
                        {
                            faceIndices.push_back(std::stoi(indexStr) - 1);
                            if (!uvIndexStr.empty())
                            {
                                faceUVIndices.push_back(std::stoi(uvIndexStr) - 1);
                            }
                        }
                        catch (...)
                        {
                            // Ignore invalid indices
                        }
                    }
                }

                if (faceIndices.size() >= 3)
                {
                    meshData.indices.push_back(faceIndices[0]);
                    meshData.indices.push_back(faceIndices[1]);
                    meshData.indices.push_back(faceIndices[2]);

                    if (faceIndices.size() == 4)
                    {
                        meshData.indices.push_back(faceIndices[0]);
                        meshData.indices.push_back(faceIndices[2]);
                        meshData.indices.push_back(faceIndices[3]);
                    }
                }

                if (faceUVIndices.size() >= 3)
                {
                    meshData._textureIndices.push_back(faceUVIndices[0]);
                    meshData._textureIndices.push_back(faceUVIndices[1]);
                    meshData._textureIndices.push_back(faceUVIndices[2]);

                    if (faceUVIndices.size() == 4)
                    {
                        meshData._textureIndices.push_back(faceUVIndices[0]);
                        meshData._textureIndices.push_back(faceUVIndices[2]);
                        meshData._textureIndices.push_back(faceUVIndices[3]);
                    }
                }
            }
            else if (prefix == "vt")
            {
                Vector2f t;
                ss >> t.x >> t.y;
                tempTextures.push_back(t);
            }
        }

        for (const auto& [x, y, z] : tempVertices) {
            meshData.vertices.insert(meshData.vertices.end(), {x, y, z});
        }

        for (const auto& [x, y] : tempTextures) {
            meshData.uvs.insert(meshData.uvs.end(), {x, y});
        }
        std::cout << "[GLEWSFMLRenderer] Loaded mesh " << path << " with " << meshData.vertices.size() / 3 << " vertices, " << meshData.uvs.size() / 2 << " UVs, and " << meshData._textureIndices.size() << " UV indices." << std::endl; // debug
        _meshCache[path] = meshData;
    }

    [[nodiscard]] GLuint GLEWSFMLRenderer::loadTexture(const std::string &path)
    {
        if (const auto it = _textureCache.find(path); it != _textureCache.end())
            return it->second;

        sf::Image image;
        if (!image.loadFromFile(path)) {
            std::cerr << "Failed to load texture: " << path << std::endl;
            return 0;
        }

        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.getSize().x, image.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return _textureCache[path] = textureID;
    }

    GLuint GLEWSFMLRenderer::createTextTexture(const std::string &text, const std::string &fontPath, unsigned int fontSize, Vector3f color)
    {
        // 1. Release current OpenGL context to avoid conflicts with SFML
#ifdef _WIN32
        wglMakeCurrent(NULL, NULL);
#else
        if (_hdc) {
            glXMakeCurrent((Display*)_hdc, None, NULL);
        }

#endif

        sf::Image textImage;
        bool success = false;

        // Scope for SFML rendering
        {
            // ... (SFML logic unchanged) ...
            if (_fontCache.find(fontPath) == _fontCache.end())
            {
                sf::Font font;
                if (!font.openFromFile(fontPath))
                {
                    std::cerr << "[GLEWSFMLRenderer] FATAL: Failed to load font: " << fontPath << std::endl;
                    // Will restore context at end of function
                }
                else
                {
                    // std::cout << "[GLEWSFMLRenderer] Font loaded: " << fontPath << std::endl;
                    _fontCache[fontPath] = font;
                }
            }

            // Only proceed if font loaded
            if (_fontCache.find(fontPath) != _fontCache.end()) {
                sf::Text sfText(_fontCache[fontPath]);
                sfText.setString(text);
                sfText.setCharacterSize(fontSize);
                sfText.setFillColor(sf::Color(color.x * 255, color.y * 255, color.z * 255));

                sf::FloatRect bounds = sfText.getLocalBounds();
                unsigned int width = (unsigned int)std::ceil(bounds.size.x + bounds.position.x);
                unsigned int height = (unsigned int)std::ceil(bounds.size.y + bounds.position.y);

                if (width == 0) width = 1;
                if (height == 0) height = 1;

                sf::RenderTexture renderTexture;
                if (renderTexture.resize({width, height}))
                {
                    renderTexture.clear(sf::Color::Transparent);
                    renderTexture.draw(sfText);
                    renderTexture.display();
                    textImage = renderTexture.getTexture().copyToImage();
                    success = true;
                }
            }
        }

        // 2. Do NOT restore context here.
        // We leave it released (None). The main loop() will re-acquire it at the start of the next frame.
        // This avoids switching contexts back and forth multiple times or conflicting with SFML's cleanup.

        // Ensure we explicitly release if SFML left something active (though SFML usually cleans up)
#ifdef _WIN32
        wglMakeCurrent(NULL, NULL);
#else
        // Note: We cannot release SFML's context because we don't have its Display*.
        // But we can ensure *our* Display has no context active (it already shouldn't).
        // Best bet: Do nothing and let loop() call MakeCurrent.
#endif

        if (!success) return 0;

        // Note: Generating textures (glGenTextures) requires an ACTIVE context!
        // We just released it or let SFML release it.
        // Wait, if context is not active, glGenTextures will FAIL or generate ID 0 or crash.

        // CRITICAL FIX: We MUST have OUR context active to upload the texture data into OUR OpenGL objects.
        // SFML created the image in CPU memory (textImage). We need to upload it to GPU in OUR context.

        // So we MUST restore context. The previous crash "BadAccess" implies we failed to restore it.
        // Let's try to release SFML's hold first? No easy way.

        // Let's try to re-acquire properly.
#ifdef _WIN32
        if (_hglrc && _hdc) wglMakeCurrent((HDC)_hdc, (HGLRC)_hglrc);
#else
        if (_hdc && _hglrc && _hwnd) {
            // Try to release current (SFML's?) just in case implicit switch fails?
            // glXMakeCurrent((Display*)_hdc, None, NULL); // No, this only affects _hdc.

            // Force activate ours
            if (!glXMakeCurrent((Display*)_hdc, (Window)_hwnd, (GLXContext)_hglrc)) {
                std::cerr << "[GLEWSFMLRenderer] FATAL: Failed to restore GLX context in createTextTexture" << std::endl;
                return 0;
            }
        }
#endif

        // Create texture in the main context (NOW ACTIVE)
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textImage.getSize().x, textImage.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, textImage.getPixelsPtr());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // NOW release it to be clean for loop()
        // CHANGE: Keep it active!
/*
#ifdef _WIN32
        wglMakeCurrent(NULL, NULL);
#else
        if (_hdc) glXMakeCurrent((Display*)_hdc, None, NULL);
#endif
*/
        return textureID;
    }
    void GLEWSFMLRenderer::cleanup()
    {
        stop();

        auto closeSocket = [](std::unique_ptr<zmq::socket_t>& socket) {
            if (socket && (void*)*socket != nullptr) {
                try {
                    socket->set(zmq::sockopt::linger, 0);
                    socket->close();
                } catch (...) {
                }
            }
        };

        closeSocket(_publisher);
        closeSocket(_subscriber);

#ifdef _WIN32
        wglMakeCurrent((HDC)_hdc, (HGLRC)_hglrc);
#else
        if (_hdc && _hglrc) glXMakeCurrent((Display*)_hdc, (Window)_hwnd, (GLXContext)_hglrc);
#endif
        destroyFramebuffer();
#ifdef _WIN32
        wglMakeCurrent(NULL, NULL);
#else
        if (_hdc) glXMakeCurrent((Display*)_hdc, None, NULL);
#endif

#ifdef _WIN32
        if (_hglrc)
        {
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext((HGLRC)_hglrc);
            _hglrc = nullptr;
        }
        if (_hdc && _hwnd)
        {
            ReleaseDC((HWND)_hwnd, (HDC)_hdc);
            _hdc = nullptr;
        }
        if (_hwnd)
        {
            DestroyWindow((HWND)_hwnd);
            _hwnd = nullptr;
        }
#else
        if (_hdc)
        {
            Display *display = (Display *)_hdc;
            if (_hglrc)
            {
                glXMakeCurrent(display, None, NULL);
                glXDestroyContext(display, (GLXContext)_hglrc);
                _hglrc = nullptr;
            }
            if (_hwnd)
            {
                XDestroyWindow(display, (Window)_hwnd);
                _hwnd = nullptr;
            }
            XCloseDisplay(display);
            _hdc = nullptr;
        }
#endif
    }

    void GLEWSFMLRenderer::createFramebuffer()
    {
        if (!_glewInitialized)
            return;
        glGenFramebuffers(1, &_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);

        glGenTextures(1, &_renderTexture);
        glBindTexture(GL_TEXTURE_2D, _renderTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _resolution.x, _resolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glGenRenderbuffers(1, &_depthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _depthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _resolution.x, _resolution.y);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBuffer);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _renderTexture, 0);
        GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, DrawBuffers);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Framebuffer is not complete!" << std::endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLEWSFMLRenderer::destroyFramebuffer()
    {
        if (_framebuffer) { glDeleteFramebuffers(1, &_framebuffer); _framebuffer = 0; }
        if (_renderTexture) { glDeleteTextures(1, &_renderTexture); _renderTexture = 0; }
        if (_depthBuffer) { glDeleteRenderbuffers(1, &_depthBuffer); _depthBuffer = 0; }
    }

    void GLEWSFMLRenderer::clearBuffer()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GLEWSFMLRenderer::render()
    {
        if (!_glewInitialized)
            return;

        auto now = std::chrono::steady_clock::now();

        if (_pendingResize)
        {
            _resolution = _newResolution;
            _hudResolution = _newResolution;  // Also update HUD resolution
            _pixelBuffer.resize(_resolution.x * _resolution.y);
            destroyFramebuffer();
            createFramebuffer();
            _pendingResize = false;
            std::cout << "[GLEWSFMLRenderer] Resized to " << _resolution.x << "x" << _resolution.y << std::endl;
        }

        float dt = std::chrono::duration<float>(now - _lastFrameTime).count();
        _lastFrameTime = now;
        
        // Clamp dt to avoid physics explosion (max 100ms)
        if (dt > 0.1f) dt = 0.1f;

        updateParticles(dt);

        glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        // Simple perspective
        float aspect = (float)_resolution.x / (float)_resolution.y;
        float fov = 60.0f * (3.14159f / 180.0f);
        float zNear = 0.1f;
        float zFar = 2000.0f; // Increased from 100.0f to see objects at z=0 from z=520
        float f = 1.0f / tan(fov / 2.0f);

        float m[16] = {0};
        m[0] = f / aspect;
        m[5] = f;
        m[10] = (zFar + zNear) / (zNear - zFar);
        m[11] = -1.0f;
        m[14] = (2.0f * zFar * zNear) / (zNear - zFar);

        glMultMatrixf(m);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Update camera from render objects if active
        if (!_activeCameraId.empty() && _renderObjects.find(_activeCameraId) != _renderObjects.end())
        {
            _cameraPos = _renderObjects[_activeCameraId].position;
            _cameraRot = _renderObjects[_activeCameraId].rotation;
        }

        // Apply camera rotation (convert radians to degrees)
        glRotatef(-_cameraRot.x * 180.0f / 3.14159f, 1.0f, 0.0f, 0.0f);
        glRotatef(-_cameraRot.y * 180.0f / 3.14159f, 0.0f, 1.0f, 0.0f);
        glRotatef(-_cameraRot.z * 180.0f / 3.14159f, 0.0f, 0.0f, 1.0f);
        glTranslatef(-_cameraPos.x, -_cameraPos.y, -_cameraPos.z);

        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);

        GLfloat lightPos[] = {0.0f, 10.0f, 0.0f, 0.0f};
        GLfloat lightDiffuse[] = {_lightColor.x, _lightColor.y, _lightColor.z, 1.0f};
        GLfloat lightAmbient[] = {0.4f, 0.4f, 0.4f, 1.0f};
        GLfloat lightSpecular[] = {0.3f, 0.3f, 0.3f, 1.0f};

        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
        glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
        glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

        for (const auto &pair : _renderObjects)
        {
            const auto &obj = pair.second;
            if (obj.isScreenSpace)
                continue;

            glPushMatrix();

            glTranslatef(obj.position.x, obj.position.y, obj.position.z);
            glRotatef(obj.rotation.x * 180.0f / 3.14159f, 1.0f, 0.0f, 0.0f);
            glRotatef(obj.rotation.y * 180.0f / 3.14159f, 0.0f, 1.0f, 0.0f);
            glRotatef(obj.rotation.z * 180.0f / 3.14159f, 0.0f, 0.0f, 1.0f);
            glScalef(obj.scale.x, obj.scale.y, obj.scale.z);

            if (obj.isSprite)
            {
                GLuint tex = 0;
                if (obj.isText)
                {
                    tex = obj.textureID;
                }
                else
                {
                    tex = loadTexture(obj.texturePath);
                }

                if (tex)
                {
                    glDisable(GL_LIGHTING); // Disable lighting for sprites/text
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, tex);
                    glColor3f(obj.color.x, obj.color.y, obj.color.z);

                    float w = 0.5f;
                    float h = 0.5f;

                    if (obj.isText)
                    {
                        int texW = 0, texH = 0;
                        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texW);
                        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texH);
                        if (texH > 0)
                        {
                            float aspect = (float)texW / (float)texH;
                            w = aspect * 0.5f;
                        }
                    }

                    glBegin(GL_QUADS);
                    glNormal3f(0.0f, 0.0f, 1.0f);
                    glTexCoord2f(0, 1);
                    glVertex3f(-w, -h, 0.0f);
                    glTexCoord2f(1, 1);
                    glVertex3f(w, -h, 0.0f);
                    glTexCoord2f(1, 0);
                    glVertex3f(w, h, 0.0f);
                    glTexCoord2f(0, 0);
                    glVertex3f(-w, h, 0.0f);
                    glEnd();

                    glDisable(GL_TEXTURE_2D);
                    glEnable(GL_LIGHTING); // Re-enable lighting
                }
            }
            else if (_meshCache.find(obj.meshPath) != _meshCache.end())
            {
                const auto &mesh = _meshCache[obj.meshPath];
                GLuint texID;

                if (!obj.texturePath.empty())
                {
                    texID = loadTexture(obj.texturePath);
                    glDisable(GL_LIGHTING);
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, texID);

                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                    glColor3f(obj.color.x, obj.color.y, obj.color.z);
                }
                else
                {
                    glEnable(GL_LIGHTING);
                    glDisable(GL_TEXTURE_2D);
                    // Reset to modulate for non-textured objects (though disabled texture makes this moot)
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                    glColor3f(obj.color.x, obj.color.y, obj.color.z);
                }
                glBegin(GL_TRIANGLES);

                for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
                {
                    unsigned int idx0 = mesh.indices[i];
                    unsigned int idx1 = mesh.indices[i + 1];
                    unsigned int idx2 = mesh.indices[i + 2];

                    unsigned int uvIdx0 = 0;
                    unsigned int uvIdx1 = 0;
                    unsigned int uvIdx2 = 0;

                    if (i + 2 < mesh._textureIndices.size())
                    {
                        uvIdx0 = mesh._textureIndices[i];
                        uvIdx1 = mesh._textureIndices[i + 1];
                        uvIdx2 = mesh._textureIndices[i + 2];
                    }

                    if (idx0 * 3 + 2 < mesh.vertices.size() &&
                        idx1 * 3 + 2 < mesh.vertices.size() &&
                        idx2 * 3 + 2 < mesh.vertices.size())
                    {

                        float x0 = mesh.vertices[idx0 * 3];
                        float y0 = mesh.vertices[idx0 * 3 + 1];
                        float z0 = mesh.vertices[idx0 * 3 + 2];

                        float x1 = mesh.vertices[idx1 * 3];
                        float y1 = mesh.vertices[idx1 * 3 + 1];
                        float z1 = mesh.vertices[idx1 * 3 + 2];

                        float x2 = mesh.vertices[idx2 * 3];
                        float y2 = mesh.vertices[idx2 * 3 + 1];
                        float z2 = mesh.vertices[idx2 * 3 + 2];

                        // Calculate face normal
                        float ux = x1 - x0;
                        float uy = y1 - y0;
                        float uz = z1 - z0;

                        float vx = x2 - x0;
                        float vy = y2 - y0;
                        float vz = z2 - z0;

                        float nx = uy * vz - uz * vy;
                        float ny = uz * vx - ux * vz;
                        float nz = ux * vy - uy * vx;

                        float len = sqrt(nx * nx + ny * ny + nz * nz);
                        if (len > 0)
                        {
                            nx /= len;
                            ny /= len;
                            nz /= len;
                            glNormal3f(nx, ny, nz);
                        }

                        if (!obj.texturePath.empty() && uvIdx0 * 2 + 1 < mesh.uvs.size())
                        {
                            glTexCoord2f(mesh.uvs[uvIdx0 * 2], mesh.uvs[uvIdx0 * 2 + 1]);
                        }
                        glVertex3f(x0, y0, z0);
                        if (!obj.texturePath.empty() && uvIdx1 * 2 + 1 < mesh.uvs.size())
                        {
                            glTexCoord2f(mesh.uvs[uvIdx1 * 2], mesh.uvs[uvIdx1 * 2 + 1]);
                        }
                        glVertex3f(x1, y1, z1);
                        if (!obj.texturePath.empty() && uvIdx2 * 2 + 1 < mesh.uvs.size())
                        {
                            glTexCoord2f(mesh.uvs[uvIdx2 * 2], mesh.uvs[uvIdx2 * 2 + 1]);
                        }
                        glVertex3f(x2, y2, z2);
                    }
                }
                glEnd();
                glDisable(GL_TEXTURE_2D);
            }

            glPopMatrix();
        }

        renderParticles();

        // Render Debug Lines
        if (!_debugLines.empty()) {
            static int lineFrameCount = 0;
            lineFrameCount++;
            if (lineFrameCount % 60 == 0) {
                 std::cout << "[GLEWSFMLRenderer] Rendering " << _debugLines.size() << " debug lines" << std::endl;
            }
            glDisable(GL_LIGHTING);
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_DEPTH_TEST);
            glLineWidth(1.0f);
            glBegin(GL_LINES);
            for (const auto& line : _debugLines) {
                glColor3f(line.color.x, line.color.y, line.color.z);
                glVertex3f(line.start.x, line.start.y, line.start.z);
                glVertex3f(line.end.x, line.end.y, line.end.z);
            }
            glEnd();
            _debugLines.clear(); // Transient: clear after persistent render
        }

        // Render Debug Texts
        if (!_debugTexts.empty()) {
            static int textFrameCount = 0;
            textFrameCount++;
            if (textFrameCount % 60 == 0) {
                 std::cout << "[GLEWSFMLRenderer] Rendering " << _debugTexts.size() << " debug texts" << std::endl;
            }
            glDisable(GL_LIGHTING);
            glEnable(GL_TEXTURE_2D);
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            std::string fontPath = "assets/fonts/arial.ttf";
            // Ensure font loaded
            if (_fontCache.find(fontPath) == _fontCache.end())
            {
                sf::Font font;
                if (!font.openFromFile(fontPath))
                {
                    std::cerr << "Failed to load font " << fontPath << std::endl;
                }
                else
                {
                    _fontCache[fontPath] = font;
                }
            }

            if (_fontCache.find(fontPath) != _fontCache.end())
            {
                sf::Font &font = _fontCache[fontPath];
                unsigned int fontSize = 24; // Use larger font for better quality
                
                // Iterate texts
                for (const auto &dt : _debugTexts)
                {
                    // Warm up glyphs to ensure texture is up to date before binding
                    for (char c : dt.text) (void)font.getGlyph(c, fontSize, false);

                    const sf::Texture &texture = font.getTexture(fontSize);
                    glBindTexture(GL_TEXTURE_2D, texture.getNativeHandle());
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                    glPushMatrix();
                    glTranslatef(dt.position.x, dt.position.y, dt.position.z);

                    // Billboarding: Align with camera view plane
                    float modelview[16];
                    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
                    // Reset rotation part to identity to face camera
                    modelview[0] = 1; modelview[4] = 0; modelview[8] = 0;
                    modelview[1] = 0; modelview[5] = 1; modelview[9] = 0;
                    modelview[2] = 0; modelview[6] = 0; modelview[10] = 1;
                    glLoadMatrixf(modelview);

                    // Scale down (font coords are pixels)
                    float scale = 0.02f;
                    glScalef(scale, -scale, scale); // Flip Y

                    // Center text horizontally
                    float textWidth = 0;
                    for (char c : dt.text) {
                        textWidth += font.getGlyph(c, fontSize, false).advance;
                    }
                    glTranslatef(-textWidth / 2.0f, 0, 0);

                    glColor3f(1.0f, 1.0f, 1.0f); // Default white, blended with texture

                    glBegin(GL_QUADS);
                    float xPos = 0;
                    float yPos = 0; // Baseline

                    for (char c : dt.text)
                    {
                        const sf::Glyph &glyph = font.getGlyph(c, fontSize, false);

                        float x0 = xPos + glyph.bounds.position.x;
                        float y0 = yPos + glyph.bounds.position.y;
                        float x1 = x0 + glyph.bounds.size.x;
                        float y1 = y0 + glyph.bounds.size.y;

                        float u0 = (float)glyph.textureRect.position.x / (float)texture.getSize().x;
                        float v0 = (float)glyph.textureRect.position.y / (float)texture.getSize().y;
                        float u1 = (float)(glyph.textureRect.position.x + glyph.textureRect.size.x) / (float)texture.getSize().x;
                        float v1 = (float)(glyph.textureRect.position.y + glyph.textureRect.size.y) / (float)texture.getSize().y;

                        glTexCoord2f(u0, v0); glVertex2f(x0, y0);
                        glTexCoord2f(u1, v0); glVertex2f(x1, y0);
                        glTexCoord2f(u1, v1); glVertex2f(x1, y1);
                        glTexCoord2f(u0, v1); glVertex2f(x0, y1);

                        xPos += glyph.advance;
                    }
                    glEnd();

                    glPopMatrix();
                }
            }
            _debugTexts.clear();
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_TEXTURE_2D);
        }

        // HUD Rendering
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, _hudResolution.x, 0, _hudResolution.y, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Sort HUD objects by zOrder
        std::vector<std::pair<int, const RenderObject*>> sortedHUD;
        for (const auto &pair : _renderObjects)
        {
            if (pair.second.isScreenSpace)
            {
                sortedHUD.push_back({pair.second.zOrder, &pair.second});
            }
        }
        std::sort(sortedHUD.begin(), sortedHUD.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

        for (const auto &sortedPair : sortedHUD)
        {
            const auto &obj = *sortedPair.second;

            // Render rectangles (button backgrounds, panels, etc.)
            if (obj.isRect)
            {
                glDisable(GL_TEXTURE_2D);
                glColor4f(obj.color.x, obj.color.y, obj.color.z, obj.alpha);

                float x = obj.position.x;
                float y = obj.position.y;
                float w = obj.scale.x;  // width
                float h = obj.scale.y;  // height

                glBegin(GL_QUADS);
                glVertex2f(x, y);
                glVertex2f(x + w, y);
                glVertex2f(x + w, y + h);
                glVertex2f(x, y + h);
                glEnd();

                // Draw outline if enabled
                if (obj.outlined)
                {
                    glLineWidth(obj.outlineWidth);
                    glColor4f(obj.outlineColor.x, obj.outlineColor.y, obj.outlineColor.z, obj.alpha);
                    glBegin(GL_LINE_LOOP);
                    glVertex2f(x, y);
                    glVertex2f(x + w, y);
                    glVertex2f(x + w, y + h);
                    glVertex2f(x, y + h);
                    glEnd();
                    glLineWidth(1.0f);
                }
            }
            // Render circles
            else if (obj.isCircle)
            {
                glDisable(GL_TEXTURE_2D);
                glColor4f(obj.color.x, obj.color.y, obj.color.z, obj.alpha);

                float cx = obj.position.x;
                float cy = obj.position.y;
                float r = obj.radius;
                int segments = obj.segments;

                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(cx, cy);  // Center
                for (int i = 0; i <= segments; i++)
                {
                    float angle = 2.0f * PI * (float)i / (float)segments;
                    glVertex2f(cx + r * cos(angle), cy + r * sin(angle));
                }
                glEnd();

                // Draw outline if enabled
                if (obj.outlined)
                {
                    glLineWidth(obj.outlineWidth);
                    glColor4f(obj.outlineColor.x, obj.outlineColor.y, obj.outlineColor.z, obj.alpha);
                    glBegin(GL_LINE_LOOP);
                    for (int i = 0; i < segments; i++)
                    {
                        float angle = 2.0f * PI * (float)i / (float)segments;
                        glVertex2f(cx + r * cos(angle), cy + r * sin(angle));
                    }
                    glEnd();
                    glLineWidth(1.0f);
                }
            }
            // Render rounded rectangles
            else if (obj.isRoundedRect)
            {
                glDisable(GL_TEXTURE_2D);
                glColor4f(obj.color.x, obj.color.y, obj.color.z, obj.alpha);

                float x = obj.position.x;
                float y = obj.position.y;
                float w = obj.scale.x;
                float h = obj.scale.y;
                float r = obj.cornerRadius;  // Already clamped when set
                // Use user-provided segments when available, fallback to 8 for compatibility
                int cornerSegments = (obj.segments > 0) ? obj.segments : 8;  // segments per corner

                // Draw central body (3 distinct quads to avoid overdraw/artifacts)
                glBegin(GL_QUADS);
                // Center vertical strip (top to bottom)
                glVertex2f(x + r, y);
                glVertex2f(x + w - r, y);
                glVertex2f(x + w - r, y + h);
                glVertex2f(x + r, y + h);
                // Left side strip
                glVertex2f(x, y + r);
                glVertex2f(x + r, y + r);
                glVertex2f(x + r, y + h - r);
                glVertex2f(x, y + h - r);
                // Right side strip
                glVertex2f(x + w - r, y + r);
                glVertex2f(x + w, y + r);
                glVertex2f(x + w, y + h - r);
                glVertex2f(x + w - r, y + h - r);
                glEnd();

                // Draw corners (Fans)
                // Bottom-left
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(x + r, y + r);
                for (int i = 0; i <= cornerSegments; i++) {
                    float angle = PI + (PI / 2.0f) * (float)i / (float)cornerSegments;
                    glVertex2f(x + r + r * cos(angle), y + r + r * sin(angle));
                }
                glEnd();

                // Bottom-right
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(x + w - r, y + r);
                for (int i = 0; i <= cornerSegments; i++) {
                    float angle = 1.5f * PI + (PI / 2.0f) * (float)i / (float)cornerSegments;
                    glVertex2f(x + w - r + r * cos(angle), y + r + r * sin(angle));
                }
                glEnd();

                // Top-right
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(x + w - r, y + h - r);
                for (int i = 0; i <= cornerSegments; i++) {
                    float angle = 0.0f + (PI / 2.0f) * (float)i / (float)cornerSegments;
                    glVertex2f(x + w - r + r * cos(angle), y + h - r + r * sin(angle));
                }
                glEnd();

                // Top-left
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(x + r, y + h - r);
                for (int i = 0; i <= cornerSegments; i++) {
                    float angle = 0.5f * PI + (PI / 2.0f) * (float)i / (float)cornerSegments;
                    glVertex2f(x + r + r * cos(angle), y + h - r + r * sin(angle));
                }
                glEnd();

                // Draw outline if enabled
                if (obj.outlined)
                {
                    glLineWidth(obj.outlineWidth);
                    glColor4f(obj.outlineColor.x, obj.outlineColor.y, obj.outlineColor.z, obj.alpha);
                    glBegin(GL_LINE_LOOP);
                    // Same corners but as line loop
                    for (int i = 0; i <= cornerSegments; i++)
                    {
                        float angle = PI + (PI / 2.0f) * (float)i / (float)cornerSegments;
                        glVertex2f(x + r + r * cos(angle), y + r + r * sin(angle));
                    }
                    for (int i = 0; i <= cornerSegments; i++)
                    {
                        float angle = PI * 1.5f + (PI / 2.0f) * (float)i / (float)cornerSegments;
                        glVertex2f(x + w - r + r * cos(angle), y + r + r * sin(angle));
                    }
                    for (int i = 0; i <= cornerSegments; i++)
                    {
                        float angle = 0.0f + (PI / 2.0f) * (float)i / (float)cornerSegments;
                        glVertex2f(x + w - r + r * cos(angle), y + h - r + r * sin(angle));
                    }
                    for (int i = 0; i <= cornerSegments; i++)
                    {
                        float angle = PI / 2.0f + (PI / 2.0f) * (float)i / (float)cornerSegments;
                        glVertex2f(x + r + r * cos(angle), y + h - r + r * sin(angle));
                    }
                    glEnd();
                    glLineWidth(1.0f);
                }
            }
            // Render lines
            else if (obj.isLine)
            {
                glDisable(GL_TEXTURE_2D);
                glLineWidth(obj.lineWidth);
                glColor4f(obj.color.x, obj.color.y, obj.color.z, obj.alpha);

                glBegin(GL_LINES);
                glVertex2f(obj.position.x, obj.position.y);
                glVertex2f(obj.endPosition.x, obj.endPosition.y);
                glEnd();

                glLineWidth(1.0f);
            }
            // Render sprites and text
            else if (obj.isSprite)
            {
                GLuint tex = 0;
                if (obj.isText)
                {
                    tex = obj.textureID;
                }
                else
                {
                    tex = loadTexture(obj.texturePath);
                }

                if (tex)
                {
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, tex);
                    glColor4f(obj.color.x, obj.color.y, obj.color.z, obj.alpha);

                    int texW = 0, texH = 0;
                    glBindTexture(GL_TEXTURE_2D, tex);
                    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texW);
                    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texH);

                    // Use explicit scale if provided, otherwise use texture size
                    float w = (obj.scale.x > 0) ? obj.scale.x : (float)texW;
                    float h = (obj.scale.y > 0) ? obj.scale.y : (float)texH;

                    // For text, scale is typically 1.0, so use texture size
                    if (obj.isText)
                    {
                        w = (float)texW * obj.scale.x;
                        h = (float)texH * obj.scale.y;
                    }

                    float x = obj.position.x;
                    float y = obj.position.y;

                    glBegin(GL_QUADS);
                    glTexCoord2f(0, 1);
                    glVertex2f(x, y);
                    glTexCoord2f(1, 1);
                    glVertex2f(x + w, y);
                    glTexCoord2f(1, 0);
                    glVertex2f(x + w, y + h);
                    glTexCoord2f(0, 0);
                    glVertex2f(x, y + h);
                    glEnd();

                    glDisable(GL_TEXTURE_2D);
                }
            }
        }

        // ═══════════════════════════════════════════════════════════════
        // DEBUG OVERLAY (Persistent Info)
        // ═══════════════════════════════════════════════════════════════
        {
            float padding = 10.0f;
            float lineOffset = 20.0f;
            unsigned int fontSize = 24;

            glEnable(GL_TEXTURE_2D);
            glDisable(GL_LIGHTING);
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            std::string fontPath = "assets/fonts/arial.ttf";
            if (_fontCache.find(fontPath) == _fontCache.end()) {
                sf::Font font;
                if (font.openFromFile(fontPath)) _fontCache[fontPath] = font;
            }

            if (_fontCache.find(fontPath) != _fontCache.end()) {
                sf::Font &font = _fontCache[fontPath];
                // Calculate DT for FPS since we don't have it as arg
                static auto lastFrameTime = std::chrono::high_resolution_clock::now();
                auto currentFrameTime = std::chrono::high_resolution_clock::now();
                float localDt = std::chrono::duration<float>(currentFrameTime - lastFrameTime).count();
                lastFrameTime = currentFrameTime;
                if (localDt <= 0.0f) localDt = 0.016f; // Prevent div by zero

                // Prepare Strings
                std::vector<std::string> lines;
                lines.push_back("FPS: " + std::to_string((int)(1.0f / localDt)));
                lines.push_back("Entities: " + std::to_string(_renderObjects.size()));
                lines.push_back("Cam Pos: " + std::to_string((int)_cameraPos.x) + ", " + std::to_string((int)_cameraPos.y) + ", " + std::to_string((int)_cameraPos.z));
                // lines.push_back("Cam Rot: " + std::to_string((int)_cameraRot.x) + ", " + std::to_string((int)_cameraRot.y) + ", " + std::to_string((int)_cameraRot.z));

                // Logging to console every ~1 second
                static float logTimer = 0.0f;
                logTimer += localDt;
                if (logTimer >= 1.0f) {
                    std::cout << "[DebugOverlay] FPS: " << (int)(1.0f / localDt) 
                              << " | Entities: " << _renderObjects.size() 
                              << " | Cam: " << (int)_cameraPos.x << "," << (int)_cameraPos.y << "," << (int)_cameraPos.z 
                              << std::endl;
                    logTimer = 0.0f;
                }

                // 1. Warm up glyphs so texture atlas is complete BEFORE binding
                float maxWidth = 0.0f;
                for (const auto& txt : lines) {
                    float w = 0.0f;
                    for (char c : txt) {
                         const sf::Glyph &g = font.getGlyph(c, fontSize, false);
                         w += g.advance;
                    }
                    if (w > maxWidth) maxWidth = w;
                }

                // 2. Draw Background Box
                // Top-Left coords: (padding, _hudResolution.y - padding)
                // Width: maxWidth + padding*2
                // Height: lines.size() * lineOffset + padding*2
                {
                    glDisable(GL_TEXTURE_2D);
                    glColor4f(0.0f, 0.0f, 0.0f, 0.7f); // Semi-transparent black
                    
                    float x = 0.0f;
                    float y = _hudResolution.y; 
                    float w = maxWidth + padding * 3.0f;
                    float h = (lines.size() * (float)fontSize) + padding * 2.0f + 20.0f; // Bit of extra height

                    // Draw Quad (Bottom-Up Ortho: y is top)
                    glBegin(GL_QUADS);
                    glVertex2f(x, y);          // Top-Left
                    glVertex2f(x + w, y);      // Top-Right
                    glVertex2f(x + w, y - h);  // Bottom-Right
                    glVertex2f(x, y - h);      // Bottom-Left
                    glEnd();
                    glEnable(GL_TEXTURE_2D);
                }

                // 3. Bind Texture
                const sf::Texture &texture = font.getTexture(fontSize);
                glBindTexture(GL_TEXTURE_2D, texture.getNativeHandle());

                auto drawText2D = [&](const std::string& str, float x, float topY, const Vector3f& color) {
                    glColor4f(color.x, color.y, color.z, 1.0f);
                    glBegin(GL_QUADS);
                    float xPos = x;
                    // topY is the visual top line.
                    
                    for (char c : str) {
                        const sf::Glyph &glyph = font.getGlyph(c, fontSize, false);
                        
                        // GL coords (Ortho 0..H). 
                        // We want to draw 'downwards' from topY.
                        // glyph.bounds.position.y is usually negative (distance from baseline up).
                        
                        // Let's use simple logic: 
                        // Start Y = topY - ascent.
                        // Actually, let's just use the screenY logic that seemed mostly correct but maybe lacked flip.
                        
                        float screenY = topY; 

                        // Standard SFML math adapted for Bottom-Up GL
                        // screenY is the "Baseline" position? No, let's treat it as the top of the line?
                        // Let's rely on bounds.
                        
                        float y0 = screenY - (glyph.bounds.position.y); // Flipped Y
                        float y1 = screenY - (glyph.bounds.position.y + glyph.bounds.size.y);

                        // But wait, glyph.bounds.position.y is roughly -Size.
                        // So screenY - (-Size) = screenY + Size (Higher up).
                        // If screenY is the BASELINE.
                        // If we want to draw BELOW the top of screen...
                        
                        // Let's try: screenY is the top of the character.
                        // y0 = screenY + ... ? 
                        
                        // Let's stick to the previous verified logic but adjust Y to be definitely visible.
                        // We are drawing at y (passed in).
                        
                        // Pos in Quad:
                        // x same.
                        // y calculated:
                        
                        // Let's use the Background box as reference. It goes from Y to Y-H.
                        // So text should be inside that.
                        
                        // Simplest: 
                        // u,v standard.
                        // x,y:
                        // x0 = xPos + glyph.bounds.left
                        // y0 = y - glyph.bounds.top  (Invert Y axis logic)
                        
                        float x0 = xPos + glyph.bounds.position.x;
                        float x1 = x0 + glyph.bounds.size.x;
                        
                        // Normal Rendering (Top-Down): y0 = y + bounds.top
                        // Bottom-Up Rendering: y0 = ViewHeight - (y + bounds.top) (?)
                        // No, our 'y' is already View coordinates (e.g. 590).
                        
                        float y_gl = topY - glyph.bounds.position.y; // If y is baseline.
                        // If y is 590. Bounds.top is -15. y_gl = 605. That's off top of screen (600).
                        
                        // We want to go DOWN.
                        // So y_gl should be LESS than 590.
                        // So y_gl = y - (-bounds.top)? No.
                        
                        // Correct for Bottom-Up with Top-Down font metrics:
                        // VertexY = CursorY - (MetricsY)
                        // But MetricsY (bounds.top) is negative for 'up'.
                        // So VertexY = CursorY + MetricsY (which is negative).
                        // e.g. 590 + (-15) = 575. Correct.
                        
                        float y0_v = topY - (glyph.bounds.position.y + glyph.bounds.size.y); // Bottom of glyph
                        float y1_v = topY - glyph.bounds.position.y; // Top of glyph check?
                        
                        // Font Texture Coords
                        float u0 = (float)glyph.textureRect.position.x / (float)texture.getSize().x;
                        float v0 = (float)glyph.textureRect.position.y / (float)texture.getSize().y;
                        float u1 = (float)(glyph.textureRect.position.x + glyph.textureRect.size.x) / (float)texture.getSize().x;
                        float v1 = (float)(glyph.textureRect.position.y + glyph.textureRect.size.y) / (float)texture.getSize().y;

                        glTexCoord2f(u0, v0); glVertex2f(x0, y1_v); // Top-Left UV -> Top-Left Vertex
                        glTexCoord2f(u1, v0); glVertex2f(x1, y1_v); // Top-Right UV -> Top-Right Vertex
                        glTexCoord2f(u1, v1); glVertex2f(x1, y0_v); // Bottom-Right UV -> Bottom-Right Vertex
                        glTexCoord2f(u0, v1); glVertex2f(x0, y0_v); // Bottom-Left UV -> Bottom-Left Vertex

                        xPos += glyph.advance;
                    }
                    glEnd();
                };

                float currentY = _hudResolution.y - padding - 20.0f; // Start slightly below top edge
                
                for (const auto& line : lines) {
                    drawText2D(line, padding + 10.0f, currentY, {1, 1, 0}); // Yellow text
                    currentY -= (float)fontSize + 5.0f;
                }
            }
             glDisable(GL_TEXTURE_2D);
        }

        glEnable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        glDisable(GL_LIGHTING);

        glReadPixels(0, 0, _resolution.x, _resolution.y, GL_RGBA, GL_UNSIGNED_BYTE, _pixelBuffer.data());

        std::vector<uint32_t> flippedBuffer(_pixelBuffer.size());
        for (unsigned int y = 0; y < _resolution.y; ++y)
        {
            for (unsigned int x = 0; x < _resolution.x; ++x)
            {
                flippedBuffer[y * _resolution.x + x] = _pixelBuffer[(_resolution.y - 1 - y) * _resolution.x + x];
            }
        }
        _pixelBuffer = flippedBuffer;

    // Prepend a small text header with the resolution so the consumer knows sizes
    std::string header;
    header.reserve(32);
    header += std::to_string(_resolution.x);
    header += ",";
    header += std::to_string(_resolution.y);
    header += ";";

    std::string pixelData(reinterpret_cast<const char *>(_pixelBuffer.data()),
                  _pixelBuffer.size() * sizeof(uint32_t));
    std::string message = header + pixelData;
    sendMessage("ImageRendered", message);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    std::vector<uint32_t> GLEWSFMLRenderer::getPixels() const
    {
        return _pixelBuffer;
    }

    Vector2u GLEWSFMLRenderer::getResolution() const
    {
        return _resolution;
    }

    void GLEWSFMLRenderer::updateParticles(float dt)
    {
        static std::mt19937 rng(std::random_device{}());
        static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        for (auto &pair : _particleGenerators)
        {
            auto &gen = pair.second;

            gen.accumulator += dt * gen.rate;
            while (gen.accumulator > 1.0f)
            {
                gen.accumulator -= 1.0f;

                Particle p;

                // Calculate world position and direction based on entity transform
                // Rotation order: Z, Y, X (matching Lua implementation)

                float radX = gen.rotation.x * 3.14159f / 180.0f;
                float radY = gen.rotation.y * 3.14159f / 180.0f;
                float radZ = gen.rotation.z * 3.14159f / 180.0f;

                float cx = cos(radX), sx = sin(radX);
                float cy = cos(radY), sy = sin(radY);
                float cz = cos(radZ), sz = sin(radZ);

                // Rotate offset
                float x = gen.offset.x;
                float y = gen.offset.y;
                float z = gen.offset.z;

                // Z rotation
                float x1 = x * cz - y * sz;
                float y1 = x * sz + y * cz;
                float z1 = z;

                // Y rotation
                float x2 = x1 * cy + z1 * sy;
                float y2 = y1;
                float z2 = -x1 * sy + z1 * cy;

                // X rotation
                float x3 = x2;
                float y3 = y2 * cx - z2 * sx;
                float z3 = y2 * sx + z2 * cx;

                p.position = {gen.position.x + x3, gen.position.y + y3, gen.position.z + z3};

                // Rotate direction
                Vector3f dir = gen.direction;

                // Z rotation
                float dx1 = dir.x * cz - dir.y * sz;
                float dy1 = dir.x * sz + dir.y * cz;
                float dz1 = dir.z;

                // Y rotation
                float dx2 = dx1 * cy + dz1 * sy;
                float dy2 = dy1;
                float dz2 = -dx1 * sy + dz1 * cy;

                // X rotation
                float dx3 = dx2;
                float dy3 = dy2 * cx - dz2 * sx;
                float dz3 = dy2 * sx + dz2 * cx;

                // Apply spread
                dx3 += dist(rng) * gen.spread;
                dy3 += dist(rng) * gen.spread;
                dz3 += dist(rng) * gen.spread;

                float len = sqrt(dx3 * dx3 + dy3 * dy3 + dz3 * dz3);
                if (len > 0)
                {
                    dx3 /= len;
                    dy3 /= len;
                    dz3 /= len;
                }

                p.velocity = {dx3 * gen.speed, dy3 * gen.speed, dz3 * gen.speed};
                p.life = gen.lifeTime;
                p.maxLife = gen.lifeTime;
                p.size = gen.size;
                p.color = gen.color;

                gen.particles.push_back(p);
            }

            for (auto it = gen.particles.begin(); it != gen.particles.end();) {
                it->life -= dt;
                if (it->life <= 0) {
                    it = gen.particles.erase(it);
                } else {
                    it->position.x += it->velocity.x * dt;
                    it->position.y += it->velocity.y * dt;
                    it->position.z += it->velocity.z * dt;
                    ++it;
                }
            }
        }
    }

    void GLEWSFMLRenderer::renderParticles()
    {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);

        for (const auto& [id, gen] : _particleGenerators) {
            for (const auto& p : gen.particles) {
                const auto alpha = (p.maxLife > 0.0f) ? p.life / p.maxLife : 0.0f;
                glColor4f(p.color.x, p.color.y, p.color.z, alpha);

                glPushMatrix();
                glTranslatef(p.position.x, p.position.y, p.position.z);

                float modelview[16];
                glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
                for (int i = 0; i < 3; i++)
                    for (int j = 0; j < 3; j++)
                        modelview[i * 4 + j] = (i == j) ? 1.0f : 0.0f;
                glLoadMatrixf(modelview);

                const auto s = p.size;
                glBegin(GL_QUADS);
                glTexCoord2f(0, 0); glVertex3f(-s, -s, 0);
                glTexCoord2f(1, 0); glVertex3f(s, -s, 0);
                glTexCoord2f(1, 1); glVertex3f(s, s, 0);
                glTexCoord2f(0, 1); glVertex3f(-s, s, 0);
                glEnd();

                glPopMatrix();
            }
        }

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    void GLEWSFMLRenderer::initContext()
    {
#ifdef _WIN32
        // Register a dummy window class
        WNDCLASSA wc = {0};
        wc.lpfnWndProc = DefWindowProcA;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "DummyOGL";
        RegisterClassA(&wc);

        // Create a dummy window
        _hwnd = (void *)CreateWindowA("DummyOGL", "Dummy", 0, 0, 0, 1, 1, NULL, NULL, wc.hInstance, NULL);
        if (!_hwnd)
        {
            std::cerr << "Failed to create dummy window" << std::endl;
            return;
        }
        _hdc = (void *)GetDC((HWND)_hwnd);

        // Set pixel format
        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            PFD_TYPE_RGBA,
            32,
            0, 0, 0, 0, 0, 0,
            0,
            0,
            0,
            0, 0, 0, 0,
            24,
            8,
            0,
            PFD_MAIN_PLANE,
            0,
            0, 0, 0};

        int pixelFormat = ChoosePixelFormat((HDC)_hdc, &pfd);
        SetPixelFormat((HDC)_hdc, pixelFormat, &pfd);

        // Create context
        _hglrc = (void *)wglCreateContext((HDC)_hdc);
        wglMakeCurrent((HDC)_hdc, (HGLRC)_hglrc);
#else
        Display *display = XOpenDisplay(NULL);
        if (!display)
        {
            std::cerr << "Failed to open X display" << std::endl;
            return;
        }

        int screen = DefaultScreen(display);
        int attribs[] = {
            GLX_RGBA,
            GLX_DEPTH_SIZE, 24,
            GLX_DOUBLEBUFFER,
            None};

        XVisualInfo *visual = glXChooseVisual(display, screen, attribs);
        if (!visual)
        {
            std::cerr << "Failed to choose visual" << std::endl;
            return;
        }

        GLXContext context = glXCreateContext(display, visual, NULL, GL_TRUE);
        if (!context)
        {
            std::cerr << "Failed to create GLX context" << std::endl;
            return;
        }

        Colormap colormap = XCreateColormap(display, RootWindow(display, screen), visual->visual, AllocNone);
        XSetWindowAttributes swa;
        swa.colormap = colormap;
        swa.event_mask = StructureNotifyMask | KeyPressMask;

        Window window = XCreateWindow(display, RootWindow(display, screen), 0, 0, 1, 1, 0, visual->depth, InputOutput, visual->visual, CWColormap | CWEventMask, &swa);

        if (!window)
        {
            std::cerr << "Failed to create X window" << std::endl;
            return;
        }

        glXMakeCurrent(display, window, context);

        _hdc = (void *)display;
        _hwnd = (void *)window;
        _hglrc = (void *)context;
#endif
    }

} // namespace rtypeEngine

// Factory function for dynamic loading
extern "C" GLEW_RENDERER_EXPORT rtypeEngine::IModule *createModule(const char *pubEndpoint, const char *subEndpoint)
{
    return new rtypeEngine::GLEWSFMLRenderer(pubEndpoint, subEndpoint);
}
