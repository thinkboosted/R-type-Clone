
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
float safeParseFloat(const std::string& str, float fallback = 0.0f) noexcept {
    errno = 0;
    char* end = nullptr;
    const float value = std::strtof(str.c_str(), &end);
    if (end == str.c_str() || errno == ERANGE) return fallback;
    return value;
}

int safeParseInt(const std::string& str, int fallback = 0) noexcept {
    errno = 0;
    char* end = nullptr;
    const long value = std::strtol(str.c_str(), &end, 10);
    if (end == str.c_str() || errno == ERANGE) return fallback;
    return static_cast<int>(value);
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
          _lastFrameTime(std::chrono::steady_clock::now()),
          _resourceManager(_hdc, _hwnd, _hglrc)
    {
        _pixelBuffer.resize(_resolution.x * _resolution.y);
    }

    void GLEWSFMLRenderer::init()
    {
        subscribe("RenderEntityCommand", [this](const std::string &msg)
                  { this->onRenderEntityCommand(msg); });
        subscribe("WindowResized", [this](const std::string &msg)
                  { this->handleWindowResized(msg); });

        initContext();

        ensureGLEWInitialized();
        createFramebuffer();

        glEnable(GL_DEPTH_TEST);

        std::cout << "[GLEWSFMLRenderer] Initialized" << std::endl;
    }

    void GLEWSFMLRenderer::ensureGLEWInitialized()
    {
        if (!_glewInitialized)
        {
            glewExperimental = GL_TRUE;
            GLenum err = glewInit();
            if (GLEW_OK != err)
            {
                std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
            }
            else
            {
                _glewInitialized = true;
                glGetError(); // Clear any error from glewInit
            }
        }
    }

    void GLEWSFMLRenderer::loop()
    {
#ifdef _WIN32
        wglMakeCurrent((HDC)_hdc, (HGLRC)_hglrc);
#else
        glXMakeCurrent((Display *)_hdc, (Window)_hwnd, (GLXContext)_hglrc);
#endif
        render();
#ifdef _WIN32
        wglMakeCurrent(NULL, NULL); // Release context
#else
        glXMakeCurrent(NULL, None, NULL); // Release context
#endif
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
                    else
                    {
                        RenderObject obj;
                        obj.id = id;
                        if (type == "cube" || type == "MESH")
                            obj.meshPath = "assets/models/cube.obj";
                        else
                            obj.meshPath = type;

                        obj.position = {0, 0, 0};
                        obj.rotation = {0, 0, 0};
                        obj.scale = {1, 1, 1};
                        obj.color = {1.0f, 0.5f, 0.2f};
                        _renderObjects[id] = obj;

                        _resourceManager.loadMesh(obj.meshPath);
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

                        obj.textureID = _resourceManager.createTextTexture(obj.text, obj.fontPath, obj.fontSize, obj.color);
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
                        obj.textureID = _resourceManager.createTextTexture(obj.text, obj.fontPath, obj.fontSize, {1.0f, 1.0f, 1.0f});
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
                    if (_particleSystem.hasGenerator(id))
                    {
                        _particleSystem.setGeneratorPosition(id, {v[0], v[1], v[2]});
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
                    if (_particleSystem.hasGenerator(id))
                    {
                        _particleSystem.setGeneratorRotation(id, {v[0], v[1], v[2]});
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
                if (_particleSystem.hasGenerator(data))
                {
                    _particleSystem.destroyGenerator(data);
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
                    _particleSystem.createGenerator(id, gen);
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

                    if (_particleSystem.hasGenerator(id))
                    {
                        auto& gen = _particleSystem.getGenerator(id);
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


    void GLEWSFMLRenderer::cleanup()
    {
        destroyFramebuffer();
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
        if (_framebuffer)
        {
            glDeleteFramebuffers(1, &_framebuffer);
            _framebuffer = 0;
        }
        if (_renderTexture)
        {
            glDeleteTextures(1, &_renderTexture);
            _renderTexture = 0;
        }
        if (_depthBuffer)
        {
            glDeleteRenderbuffers(1, &_depthBuffer);
            _depthBuffer = 0;
        }
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

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - _lastFrameTime).count();
        _lastFrameTime = now;

        _particleSystem.update(dt);

        glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
        glViewport(0, 0, _resolution.x, _resolution.y);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        // Simple perspective
        float aspect = (float)_resolution.x / (float)_resolution.y;
        float fov = 60.0f * (3.14159f / 180.0f);
        float zNear = 0.1f;
        float zFar = 100.0f;
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
                    tex = _resourceManager.loadTexture(obj.texturePath);
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
            else if (const MeshData* meshPtr = _resourceManager.getMesh(obj.meshPath))
            {
                const auto &mesh = *meshPtr;
                GLuint texID;

                if (!obj.texturePath.empty())
                {
                    texID = _resourceManager.loadTexture(obj.texturePath);
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

        _particleSystem.render();

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
                    tex = _resourceManager.loadTexture(obj.texturePath);
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


#ifdef _WIN32
    #define GLEW_RENDERER_EXPORT __declspec(dllexport)
#else
    #define GLEW_RENDERER_EXPORT
#endif

extern "C" GLEW_RENDERER_EXPORT rtypeEngine::IModule *createModule(const char *pubEndpoint, const char *subEndpoint)
{
    return new rtypeEngine::GLEWSFMLRenderer(pubEndpoint, subEndpoint);
}
