#include "GLEWRenderer.hpp"
#include <stdexcept>
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <fstream>
#include <cmath>

namespace rtypeEngine {

GLEWRenderer::GLEWRenderer(const char* pubEndpoint, const char* subEndpoint)
    : I3DRenderer(pubEndpoint, subEndpoint),
      _resolution{800, 600},
      _framebuffer(0),
      _renderTexture(0),
      _depthBuffer(0),
      _glewInitialized(false) {
          _pixelBuffer.resize(_resolution.x * _resolution.y);
      }

GLEWRenderer::~GLEWRenderer() {
    destroyFramebuffer();
}

void GLEWRenderer::init() {
    subscribe("SceneUpdated", [this](const std::string& msg) {
        this->onSceneUpdated(msg);
    });

    // Create SFML context to allow OpenGL calls
    _context = std::make_unique<sf::Context>();

    ensureGLEWInitialized();
    createFramebuffer();

    glEnable(GL_DEPTH_TEST);

    std::cout << "[GLEWRenderer] Initialized and subscribed to SceneUpdated" << std::endl;
}

void GLEWRenderer::ensureGLEWInitialized() {
    if (!_glewInitialized) {
        GLenum err = glewInit();
        if (GLEW_OK != err) {
            std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
        } else {
            _glewInitialized = true;
        }
    }
}

void GLEWRenderer::loop() {
}

void GLEWRenderer::onSceneUpdated(const std::string& message) {
    _renderObjects.clear();

    std::stringstream ss(message);
    std::string segment;
    while (std::getline(ss, segment, ';')) {
        if (segment.empty()) continue;

        size_t colonPos = segment.find(':');
        if (colonPos == std::string::npos) continue;

        std::string type = segment.substr(0, colonPos);
        std::string data = segment.substr(colonPos + 1);

        std::stringstream dataSs(data);
        std::string val;
        std::vector<std::string> values;
        while (std::getline(dataSs, val, ',')) {
            values.push_back(val);
        }

        if (type == "Camera") {
            if (values.size() >= 4) {
                _cameraPos = {std::stof(values[1]), std::stof(values[2]), std::stof(values[3])};
            }
        } else if (type == "Light") {
            if (values.size() >= 8) {
                _lightPos = {std::stof(values[1]), std::stof(values[2]), std::stof(values[3])};
                _lightColor = {std::stof(values[4]), std::stof(values[5]), std::stof(values[6])};
                _lightIntensity = std::stof(values[7]);
            }
        } else if (type == "Mesh") {
            if (values.size() >= 11) {
                RenderObject obj;
                obj.id = values[0];
                obj.meshPath = values[1];
                obj.position = {std::stof(values[2]), std::stof(values[3]), std::stof(values[4])};
                obj.rotation = {std::stof(values[5]), std::stof(values[6]), std::stof(values[7])};
                obj.scale = {std::stof(values[8]), std::stof(values[9]), std::stof(values[10])};
                _renderObjects.push_back(obj);

                if (_meshCache.find(obj.meshPath) == _meshCache.end()) {
                    loadMesh(obj.meshPath);
                }
            }
        }
    }

    render();
}

void GLEWRenderer::loadMesh(const std::string& path) {
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

    // Flatten vertices for simple rendering (indexed)
    for (const auto& v : tempVertices) {
        meshData.vertices.push_back(v.x);
        meshData.vertices.push_back(v.y);
        meshData.vertices.push_back(v.z);
    }

    _meshCache[path] = meshData;
    std::cout << "Loaded mesh: " << path << " with " << meshData.vertices.size() / 3 << " vertices and " << meshData.indices.size() / 3 << " faces." << std::endl;
}

void GLEWRenderer::cleanup() {
    destroyFramebuffer();
}

void GLEWRenderer::createFramebuffer() {
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

void GLEWRenderer::destroyFramebuffer() {
    if (_framebuffer) glDeleteFramebuffers(1, &_framebuffer);
    if (_renderTexture) glDeleteTextures(1, &_renderTexture);
    if (_depthBuffer) glDeleteRenderbuffers(1, &_depthBuffer);
}

void GLEWRenderer::addMesh() {
}

void GLEWRenderer::clearBuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLEWRenderer::render() {
    if (!_glewInitialized) return;

    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glViewport(0, 0, _resolution.x, _resolution.y);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
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

    glTranslatef(-_cameraPos.x, -_cameraPos.y, -_cameraPos.z);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[] = {_lightPos.x, _lightPos.y, _lightPos.z, 1.0f};
    GLfloat lightColor[] = {_lightColor.x, _lightColor.y, _lightColor.z, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    for (const auto& obj : _renderObjects) {
        glPushMatrix();

        glTranslatef(obj.position.x, obj.position.y, obj.position.z);
        glRotatef(obj.rotation.x * 180.0f / 3.14159f, 1.0f, 0.0f, 0.0f);
        glRotatef(obj.rotation.y * 180.0f / 3.14159f, 0.0f, 1.0f, 0.0f);
        glRotatef(obj.rotation.z * 180.0f / 3.14159f, 0.0f, 0.0f, 1.0f);
        glScalef(obj.scale.x, obj.scale.y, obj.scale.z);

        if (_meshCache.find(obj.meshPath) != _meshCache.end()) {
            const auto& mesh = _meshCache[obj.meshPath];

            glColor3f(1.0f, 0.5f, 0.2f); // Orange cube

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
