#include "GLEWSFMLRenderer.hpp"
#ifdef _WIN32
#include <windows.h>
#endif
#include <stdexcept>
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <fstream>
#include <cmath>

namespace rtypeEngine {

GLEWSFMLRenderer::GLEWSFMLRenderer(const char* pubEndpoint, const char* subEndpoint)
    : I3DRenderer(pubEndpoint, subEndpoint),
      _resolution{800, 600},
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
      _lightIntensity(1.0f) {
          _pixelBuffer.resize(_resolution.x * _resolution.y);
      }

GLEWSFMLRenderer::~GLEWSFMLRenderer() {
    destroyFramebuffer();
}

void GLEWSFMLRenderer::init() {
    subscribe("RenderEntityCommand", [this](const std::string& msg) {
        this->onRenderEntityCommand(msg);
    });

    initContext();

    ensureGLEWInitialized();
    createFramebuffer();

    glEnable(GL_DEPTH_TEST);

    std::cout << "[GLEWSFMLRenderer] Initialized" << std::endl;
}

void GLEWSFMLRenderer::ensureGLEWInitialized() {
    if (!_glewInitialized) {
        GLenum err = glewInit();
        if (GLEW_OK != err) {
            std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
        } else {
            _glewInitialized = true;
        }
    }
}

void GLEWSFMLRenderer::loop() {
    render();
}

void GLEWSFMLRenderer::onRenderEntityCommand(const std::string& message) {
    std::stringstream ss(message);
    std::string segment;
    while (std::getline(ss, segment, ';')) {
        if (segment.empty()) continue;

        size_t colonPos = segment.find(':');
        if (colonPos == std::string::npos) continue;

        std::string command = segment.substr(0, colonPos);
        std::string data = segment.substr(colonPos + 1);

        if (command == "CreateEntity") {
             size_t split = data.find(':');
             if (split != std::string::npos) {
                 std::string type = data.substr(0, split);
                 std::string id = data.substr(split + 1);

                 if (type == "Camera" || type == "CAMERA") {
                     RenderObject obj;
                     obj.id = id;
                     obj.meshPath = "";
                     obj.position = {0,0,0};
                     obj.rotation = {0,0,0};
                     obj.scale = {1,1,1};
                     obj.color = {1.0f, 1.0f, 1.0f};
                     _renderObjects[id] = obj;
                 } else if (type == "Light" || type == "LIGHT") {
                     _activeLightId = id;
                 } else if (type == "Sprite" || type == "SPRITE") {
                     size_t split2 = id.find(':');
                     if (split2 != std::string::npos) {
                         std::string texturePath = id.substr(0, split2);
                         std::string realId = id.substr(split2 + 1);

                         RenderObject obj;
                         obj.id = realId;
                         obj.isSprite = true;
                         obj.texturePath = texturePath;
                         obj.position = {0,0,0};
                         obj.rotation = {0,0,0};
                         obj.scale = {1,1,1};
                         obj.color = {1.0f, 1.0f, 1.0f};
                         _renderObjects[realId] = obj;
                         loadTexture(texturePath);
                     }
                 } else if (type == "HUDSprite" || type == "HUDSPRITE") {
                     size_t split2 = id.find(':');
                     if (split2 != std::string::npos) {
                         std::string texturePath = id.substr(0, split2);
                         std::string realId = id.substr(split2 + 1);

                         RenderObject obj;
                         obj.id = realId;
                         obj.isSprite = true;
                         obj.isScreenSpace = true;
                         obj.texturePath = texturePath;
                         obj.position = {0,0,0};
                         obj.rotation = {0,0,0};
                         obj.scale = {1,1,1};
                         obj.color = {1.0f, 1.0f, 1.0f};
                         _renderObjects[realId] = obj;
                         loadTexture(texturePath);
                     }
                 } else {
                     RenderObject obj;
                     obj.id = id;
                     if (type == "cube" || type == "MESH") obj.meshPath = "assets/models/cube.obj";
                     else obj.meshPath = type;

                     obj.position = {0,0,0};
                     obj.rotation = {0,0,0};
                     obj.scale = {1,1,1};
                     obj.color = {1.0f, 0.5f, 0.2f};
                     _renderObjects[id] = obj;

                     if (_meshCache.find(obj.meshPath) == _meshCache.end()) {
                        loadMesh(obj.meshPath);
                     }
                 }
             }
        } else if (command == "CreateText") {
             size_t split = data.find(':');
             if (split != std::string::npos) {
                 std::string id = data.substr(0, split);
                 std::string rest = data.substr(split + 1);

                 std::stringstream ss2(rest);
                 std::string textContent, fontPath, fontSizeStr, isScreenSpaceStr;
                 std::getline(ss2, fontPath, ':');
                 std::getline(ss2, fontSizeStr, ':');
                 std::getline(ss2, isScreenSpaceStr, ':');
                 std::getline(ss2, textContent);

                 try {
                     RenderObject obj;
                     obj.id = id;
                     obj.isText = true;
                     obj.isSprite = true;
                     obj.text = textContent;
                     obj.fontPath = fontPath;
                     obj.fontSize = std::stoi(fontSizeStr);
                     obj.isScreenSpace = (isScreenSpaceStr == "1" || isScreenSpaceStr == "true");
                     obj.position = {0,0,0};
                     obj.rotation = {0,0,0};
                     obj.scale = {1,1,1};
                     obj.color = {1.0f, 1.0f, 1.0f};

                     obj.textureID = createTextTexture(obj.text, obj.fontPath, obj.fontSize, obj.color);
                     _renderObjects[id] = obj;
                     std::cout << "[GLEWSFMLRenderer] Created text: " << id << " '" << textContent << "'" << std::endl;
                 } catch (const std::exception& e) {
                     std::cerr << "[GLEWSFMLRenderer] Error creating text: " << e.what() << std::endl;
                 }
             }
        } else if (command == "SetText") {
            size_t split = data.find(':');
            if (split != std::string::npos) {
                std::string id = data.substr(0, split);
                std::string newText = data.substr(split + 1);

                if (_renderObjects.find(id) != _renderObjects.end()) {
                    auto& obj = _renderObjects[id];
                    if (obj.isText) {
                        obj.text = newText;
                        if (obj.textureID) glDeleteTextures(1, &obj.textureID);
                        obj.textureID = createTextTexture(obj.text, obj.fontPath, obj.fontSize, obj.color);
                    }
                }
            }
        } else if (command == "SetPosition") {
            std::stringstream dss(data);
            std::string id, val;
            std::getline(dss, id, ',');
            std::vector<float> v;
            while(std::getline(dss, val, ',')) v.push_back(std::stof(val));

            if (v.size() >= 3) {
                if (_renderObjects.find(id) != _renderObjects.end()) {
                    _renderObjects[id].position = {v[0], v[1], v[2]};
                } else if (id == _activeCameraId) {
                    _cameraPos = {v[0], v[1], v[2]};
                } else if (id == _activeLightId) {
                    _lightPos = {v[0], v[1], v[2]};
                }
            }
        } else if (command == "SetRotation") {
            std::stringstream dss(data);
            std::string id, val;
            std::getline(dss, id, ',');
            std::vector<float> v;
            while(std::getline(dss, val, ',')) v.push_back(std::stof(val));

            if (v.size() >= 3) {
                if (_renderObjects.find(id) != _renderObjects.end()) {
                    _renderObjects[id].rotation = {v[0], v[1], v[2]};
                } else if (id == _activeCameraId) {
                    _cameraRot = {v[0], v[1], v[2]};
                }
            }
        } else if (command == "SetScale") {
            std::stringstream dss(data);
            std::string id, val;
            std::getline(dss, id, ',');
            std::vector<float> v;
            while(std::getline(dss, val, ',')) v.push_back(std::stof(val));

            if (v.size() >= 3 && _renderObjects.find(id) != _renderObjects.end()) {
                _renderObjects[id].scale = {v[0], v[1], v[2]};
            }
        } else if (command == "SetColor") {
            std::stringstream dss(data);
            std::string id, val;
            std::getline(dss, id, ',');
            std::vector<float> v;
            while(std::getline(dss, val, ',')) v.push_back(std::stof(val));

            if (v.size() >= 3 && _renderObjects.find(id) != _renderObjects.end()) {
                _renderObjects[id].color = {v[0], v[1], v[2]};
            }
        } else if (command == "SetLightProperties") {
            std::stringstream dss(data);
            std::string id, val;
            std::getline(dss, id, ',');
            std::vector<float> v;
            while(std::getline(dss, val, ',')) v.push_back(std::stof(val));

            if (v.size() >= 4) {
                _lightColor = {v[0], v[1], v[2]};
                _lightIntensity = v[3];
            }
        } else if (command == "SetActiveCamera") {
            _activeCameraId = data;
        } else if (command == "DestroyEntity") {
            if (_renderObjects.find(data) != _renderObjects.end()) {
                _renderObjects.erase(data);
            }
        }
    }
}

void GLEWSFMLRenderer::loadMesh(const std::string& path) {
    if (path.empty()) return;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open mesh file: " << path << std::endl;
        return;
    }

    MeshData meshData;
    std::string line;
    std::vector<Vector3f> tempVertices;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v") {
            Vector3f v;
            ss >> v.x >> v.y >> v.z;
            tempVertices.push_back(v);
        } else if (prefix == "f") {
            unsigned int vIndex[3];
            // Simple OBJ parser for triangulated faces (v1 v2 v3)
            // Note: OBJ indices are 1-based
            for (int i = 0; i < 3; ++i) {
                ss >> vIndex[i];
                // Handle texture/normal indices if present (e.g., 1/1/1)
                // For this simple parser, we assume just vertex indices or space separated
                // If there are slashes, we need to ignore them.
                // But for the cube.obj I created, it's just "f 1 2 3"

                // If the file has slashes, this simple parsing might fail or need adjustment.
                // Let's assume the simple format I wrote.
                meshData.indices.push_back(vIndex[i] - 1);
            }
        }
    }

    for (const auto& v : tempVertices) {
        meshData.vertices.push_back(v.x);
        meshData.vertices.push_back(v.y);
        meshData.vertices.push_back(v.z);
    }

    _meshCache[path] = meshData;
    std::cout << "[GLEWSFMLRenderer] Loaded mesh: " << path << " with " << meshData.vertices.size() / 3 << " vertices and " << meshData.indices.size() / 3 << " faces." << std::endl;
}

GLuint GLEWSFMLRenderer::loadTexture(const std::string& path) {
    if (_textureCache.find(path) != _textureCache.end()) {
        return _textureCache[path];
    }

    sf::Image image;
    if (!image.loadFromFile(path)) {
        std::cerr << "[GLEWSFMLRenderer] Failed to load texture: " << path << std::endl;
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

    _textureCache[path] = textureID;
    std::cout << "[GLEWSFMLRenderer] Loaded texture: " << path << std::endl;
    return textureID;
}

GLuint GLEWSFMLRenderer::createTextTexture(const std::string& text, const std::string& fontPath, unsigned int fontSize, Vector3f color) {
    // Save current OpenGL context
    #ifdef _WIN32
    HDC oldDC = wglGetCurrentDC();
    HGLRC oldContext = wglGetCurrentContext();
    #endif

    sf::Image textImage;
    bool success = false;

    // Scope for SFML rendering to ensure resources are cleaned up before context restore
    {
        if (_fontCache.find(fontPath) == _fontCache.end()) {
            sf::Font font;
            if (!font.openFromFile(fontPath)) {
                std::cerr << "Failed to load font: " << fontPath << std::endl;
                // Restore context before returning
                #ifdef _WIN32
                if (oldDC && oldContext) wglMakeCurrent(oldDC, oldContext);
                #endif
                return 0;
            }
            _fontCache[fontPath] = font;
        }

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
        if (renderTexture.resize({width, height})) {
            renderTexture.clear(sf::Color::Transparent);
            renderTexture.draw(sfText);
            renderTexture.display();
            textImage = renderTexture.getTexture().copyToImage();
            success = true;
        }
    }

    // Restore main OpenGL context
    #ifdef _WIN32
    if (oldDC && oldContext) {
        wglMakeCurrent(oldDC, oldContext);
    }
    #endif

    if (!success) return 0;

    // Create texture in the main context
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textImage.getSize().x, textImage.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, textImage.getPixelsPtr());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}

void GLEWSFMLRenderer::cleanup() {
    destroyFramebuffer();
#ifdef _WIN32
    if (_hglrc) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext((HGLRC)_hglrc);
        _hglrc = nullptr;
    }
    if (_hdc && _hwnd) {
        ReleaseDC((HWND)_hwnd, (HDC)_hdc);
        _hdc = nullptr;
    }
    if (_hwnd) {
        DestroyWindow((HWND)_hwnd);
        _hwnd = nullptr;
    }
#endif
}

void GLEWSFMLRenderer::createFramebuffer() {
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

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLEWSFMLRenderer::destroyFramebuffer() {
    if (_framebuffer) glDeleteFramebuffers(1, &_framebuffer);
    if (_renderTexture) glDeleteTextures(1, &_renderTexture);
    if (_depthBuffer) glDeleteRenderbuffers(1, &_depthBuffer);
}

void GLEWSFMLRenderer::addMesh() {
}

void GLEWSFMLRenderer::clearBuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLEWSFMLRenderer::render() {
    if (!_glewInitialized) return;

    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glViewport(0, 0, _resolution.x, _resolution.y);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
    if (!_activeCameraId.empty() && _renderObjects.find(_activeCameraId) != _renderObjects.end()) {
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

    for (const auto& pair : _renderObjects) {
        const auto& obj = pair.second;
        if (obj.isScreenSpace) continue;

        glPushMatrix();

        glTranslatef(obj.position.x, obj.position.y, obj.position.z);
        glRotatef(obj.rotation.x * 180.0f / 3.14159f, 1.0f, 0.0f, 0.0f);
        glRotatef(obj.rotation.y * 180.0f / 3.14159f, 0.0f, 1.0f, 0.0f);
        glRotatef(obj.rotation.z * 180.0f / 3.14159f, 0.0f, 0.0f, 1.0f);
        glScalef(obj.scale.x, obj.scale.y, obj.scale.z);

        if (obj.isSprite) {
             GLuint tex = 0;
             if (obj.isText) {
                 tex = obj.textureID;
             } else {
                 tex = loadTexture(obj.texturePath);
             }

             if (tex) {
                 glDisable(GL_LIGHTING); // Disable lighting for sprites/text
                 glEnable(GL_TEXTURE_2D);
                 glBindTexture(GL_TEXTURE_2D, tex);
                 glColor3f(obj.color.x, obj.color.y, obj.color.z);

                 float w = 0.5f;
                 float h = 0.5f;

                 if (obj.isText) {
                     int texW = 0, texH = 0;
                     glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texW);
                     glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texH);
                     if (texH > 0) {
                        float aspect = (float)texW / (float)texH;
                        w = aspect * 0.5f;
                     }
                 }

                 glBegin(GL_QUADS);
                 glNormal3f(0.0f, 0.0f, 1.0f);
                 glTexCoord2f(0, 1); glVertex3f(-w, -h, 0.0f);
                 glTexCoord2f(1, 1); glVertex3f( w, -h, 0.0f);
                 glTexCoord2f(1, 0); glVertex3f( w,  h, 0.0f);
                 glTexCoord2f(0, 0); glVertex3f(-w,  h, 0.0f);
                 glEnd();

                 glDisable(GL_TEXTURE_2D);
                 glEnable(GL_LIGHTING); // Re-enable lighting
             }
        } else if (_meshCache.find(obj.meshPath) != _meshCache.end()) {
            const auto& mesh = _meshCache[obj.meshPath];

            glColor3f(obj.color.x, obj.color.y, obj.color.z);

            glBegin(GL_TRIANGLES);
            for (size_t i = 0; i < mesh.indices.size(); ++i) {
                unsigned int idx = mesh.indices[i];
                if (idx * 3 + 2 < mesh.vertices.size()) {
                    float nx = mesh.vertices[idx * 3];
                    float ny = mesh.vertices[idx * 3 + 1];
                    float nz = mesh.vertices[idx * 3 + 2];
                    float len = sqrt(nx*nx + ny*ny + nz*nz);
                    if (len > 0) glNormal3f(nx/len, ny/len, nz/len);

                    glVertex3f(mesh.vertices[idx * 3], mesh.vertices[idx * 3 + 1], mesh.vertices[idx * 3 + 2]);
                }
            }
            glEnd();
        }

        glPopMatrix();
    }

    // HUD Rendering
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, _resolution.x, 0, _resolution.y, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    for (const auto& pair : _renderObjects) {
        const auto& obj = pair.second;
        if (!obj.isScreenSpace) continue;

        if (obj.isSprite) {
             GLuint tex = 0;
             if (obj.isText) {
                 tex = obj.textureID;
             } else {
                 tex = loadTexture(obj.texturePath);
             }

             if (tex) {
                 glEnable(GL_TEXTURE_2D);
                 glBindTexture(GL_TEXTURE_2D, tex);
                 glColor3f(obj.color.x, obj.color.y, obj.color.z);

                 int texW = 0, texH = 0;
                 glBindTexture(GL_TEXTURE_2D, tex);
                 glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texW);
                 glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texH);

                 float w = (float)texW * obj.scale.x;
                 float h = (float)texH * obj.scale.y;

                 float x = obj.position.x;
                 float y = obj.position.y;

                 glBegin(GL_QUADS);
                 glTexCoord2f(0, 1); glVertex2f(x, y);
                 glTexCoord2f(1, 1); glVertex2f(x + w, y);
                 glTexCoord2f(1, 0); glVertex2f(x + w, y + h);
                 glTexCoord2f(0, 0); glVertex2f(x, y + h);
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
    for (unsigned int y = 0; y < _resolution.y; ++y) {
        for (unsigned int x = 0; x < _resolution.x; ++x) {
            flippedBuffer[y * _resolution.x + x] = _pixelBuffer[(_resolution.y - 1 - y) * _resolution.x + x];
        }
    }
    _pixelBuffer = flippedBuffer;

    std::string pixelData(reinterpret_cast<const char*>(_pixelBuffer.data()),
                         _pixelBuffer.size() * sizeof(uint32_t));
    sendMessage("ImageRendered", pixelData);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::vector<uint32_t> GLEWSFMLRenderer::getPixels() const {
    return _pixelBuffer;
}

Vector2u GLEWSFMLRenderer::getResolution() const {
    return _resolution;
}

void GLEWSFMLRenderer::initContext() {
#ifdef _WIN32
    // Register a dummy window class
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "DummyOGL";
    RegisterClassA(&wc);

    // Create a dummy window
    _hwnd = (void*)CreateWindowA("DummyOGL", "Dummy", 0, 0, 0, 1, 1, NULL, NULL, wc.hInstance, NULL);
    if (!_hwnd) {
        std::cerr << "Failed to create dummy window" << std::endl;
        return;
    }
    _hdc = (void*)GetDC((HWND)_hwnd);

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
        0, 0, 0
    };

    int pixelFormat = ChoosePixelFormat((HDC)_hdc, &pfd);
    SetPixelFormat((HDC)_hdc, pixelFormat, &pfd);

    // Create context
    _hglrc = (void*)wglCreateContext((HDC)_hdc);
    wglMakeCurrent((HDC)_hdc, (HGLRC)_hglrc);
#endif
}

}  // namespace rtypeEngine

// Factory function for dynamic loading
extern "C" __declspec(dllexport) rtypeEngine::IModule* createModule(const char* pubEndpoint, const char* subEndpoint) {
    return new rtypeEngine::GLEWSFMLRenderer(pubEndpoint, subEndpoint);
}
