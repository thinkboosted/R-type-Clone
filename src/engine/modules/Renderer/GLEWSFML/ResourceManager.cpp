#include "ResourceManager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#endif

namespace rtypeEngine {

    ResourceManager::ResourceManager(void*& hdc, void*& hwnd, void*& hglrc)
        : _hdc(hdc), _hwnd(hwnd), _hglrc(hglrc) {}

    const MeshData* ResourceManager::getMesh(const std::string& path) const {
        auto it = _meshCache.find(path);
        if (it != _meshCache.end()) return &it->second;
        return nullptr;
    }

    GLuint ResourceManager::getTexture(const std::string& path) const {
        auto it = _textureCache.find(path);
        if (it != _textureCache.end()) return it->second;
        return 0;
    }

    sf::Font* ResourceManager::getFont(const std::string& path) {
        auto it = _fontCache.find(path);
        if (it != _fontCache.end()) return &it->second;
        return nullptr;
    }

    void ResourceManager::loadMesh(const std::string &path)
    {
        if (path.empty())
            return;
        if (_meshCache.find(path) != _meshCache.end()) return;

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

        for (const auto &v : tempVertices)
        {
            meshData.vertices.push_back(v.x);
            meshData.vertices.push_back(v.y);
            meshData.vertices.push_back(v.z);
        }

        for (const auto &v : tempTextures)
        {
            meshData.uvs.push_back(v.x);
            meshData.uvs.push_back(v.y);
        }
        std::cout << "[ResourceManager] Loaded mesh " << path << " with " << meshData.vertices.size() / 3 << " vertices, " << meshData.uvs.size() / 2 << " UVs, and " << meshData._textureIndices.size() << " UV indices." << std::endl;
        _meshCache[path] = meshData;
    }

    GLuint ResourceManager::loadTexture(const std::string &path)
    {
        if (_textureCache.find(path) != _textureCache.end())
        {
            return _textureCache[path];
        }

        sf::Image image;
        if (!image.loadFromFile(path))
        {
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

        _textureCache[path] = textureID;
        return textureID;
    }

    GLuint ResourceManager::createTextTexture(const std::string &text, const std::string &fontPath, unsigned int fontSize, Vector3f color)
    {
// Save current OpenGL context
#ifdef _WIN32
        HDC oldDC = wglGetCurrentDC();
        HGLRC oldContext = wglGetCurrentContext();
#else
        Display* display = (Display*)_hdc;
        GLXContext oldContext = glXGetCurrentContext();
        GLXDrawable oldDrawable = glXGetCurrentDrawable();
#endif
        (void)oldContext; // suppress unused warning if not used in some paths

        sf::Image textImage;
        bool success = false;

        // Scope for SFML rendering to ensure resources are cleaned up before context restore
        {
            if (_fontCache.find(fontPath) == _fontCache.end())
            {
                sf::Font font;
                if (!font.openFromFile(fontPath))
                {
                    std::cerr << "Failed to load font: " << fontPath << std::endl;
// Restore context before returning
#ifdef _WIN32
                    if (oldDC && oldContext)
                        wglMakeCurrent(oldDC, oldContext);
#else
                    if (display && oldContext)
                        glXMakeCurrent(display, oldDrawable, oldContext);
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

            if (width == 0)
                width = 1;
            if (height == 0)
                height = 1;

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

// Activate our renderer context to ensure the texture is created in the correct context
#ifdef _WIN32
        if (_hglrc && _hdc)
        {
            wglMakeCurrent((HDC)_hdc, (HGLRC)_hglrc);
        }
#else
        // On Linux, make our GL context current for texture creation
        if (_hdc && _hwnd && _hglrc)
        {
            glXMakeCurrent(display, (Window)_hwnd, (GLXContext)_hglrc);
        }
#endif

        if (!success)
        {
// Restore original context if we failed
#ifdef _WIN32
            if (oldDC && oldContext)
                wglMakeCurrent(oldDC, oldContext);
            else
                wglMakeCurrent(NULL, NULL);
#else
            if (oldContext)
                glXMakeCurrent(display, oldDrawable, oldContext);
            else
                glXMakeCurrent(display, None, NULL);
#endif
            return 0;
        }

        // Create texture in the main context
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textImage.getSize().x, textImage.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, textImage.getPixelsPtr());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

// Restore original context
#ifdef _WIN32
        if (oldDC && oldContext)
        {
            wglMakeCurrent(oldDC, oldContext);
        }
        else
        {
            wglMakeCurrent(NULL, NULL);
        }
#else
        // Restore context on Linux - release it so loop() can acquire it
        if (oldContext)
             glXMakeCurrent(display, oldDrawable, oldContext);
        else
             glXMakeCurrent(display, None, NULL);
#endif

        return textureID;
    }

}
