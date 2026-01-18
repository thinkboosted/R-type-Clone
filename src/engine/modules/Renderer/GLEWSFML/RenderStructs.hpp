#pragma once
#include <string>
#include <vector>
#include <GL/glew.h>
#include "../I3DRenderer.hpp"

namespace rtypeEngine {
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

    struct MeshData {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        std::vector<float> uvs;
        std::vector<unsigned int> _textureIndices;
    };
}
