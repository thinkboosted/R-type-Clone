#pragma once

#include <string>
#include <fstream>
#include "../core/Logger.hpp"

namespace rtypeEngine {

struct Mesh {
    std::string modelPath;
    std::string texturePath;

    Mesh(const std::string& model = "", const std::string& texture = "")
        : modelPath(model), texturePath(texture) {
        validatePaths();
    }

    void validatePaths() const {
        if (!modelPath.empty()) {
            std::ifstream f(modelPath);
            if (!f.good()) {
                Logger::Info("[Mesh] Model not found: " + modelPath);
            }
        }
        if (!texturePath.empty()) {
            std::ifstream f(texturePath);
            if (!f.good()) {
                Logger::Info("[Mesh] Texture not found: " + texturePath);
            }
        }
    }

    bool isModelLoaded() const {
        if (modelPath.empty()) return false;
        std::ifstream f(modelPath);
        return f.good();
    }

    bool isTextureLoaded() const {
        if (texturePath.empty()) return true; // Optional texture
        std::ifstream f(texturePath);
        return f.good();
    }
};

}
