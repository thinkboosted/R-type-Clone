#pragma once

#include <map>
#include <string>
#include <GL/glew.h>
#include <SFML/Graphics.hpp>
#include "RenderStructs.hpp"

namespace rtypeEngine {
    class ResourceManager {
    public:
        ResourceManager(void*& hdc, void*& hwnd, void*& hglrc);
        ~ResourceManager() = default;

        void loadMesh(const std::string& path);
        GLuint loadTexture(const std::string& path);
        GLuint createTextTexture(const std::string& text, const std::string& fontPath, unsigned int fontSize, Vector3f color);

        const MeshData* getMesh(const std::string& path) const;
        GLuint getTexture(const std::string& path) const;
        sf::Font* getFont(const std::string& path);

    private:
        std::map<std::string, MeshData> _meshCache;
        std::map<std::string, GLuint> _textureCache;
        std::map<std::string, sf::Font> _fontCache;

        void*& _hdc;
        void*& _hwnd;
        void*& _hglrc;
    };
}
