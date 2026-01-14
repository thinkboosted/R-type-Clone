#pragma once

#include <string>
#include <array>
#include <algorithm>
#include "../core/Logger.hpp"

namespace rtypeEngine {

enum class ColliderType {
    BOX,
    SPHERE,
    CAPSULE,
    CYLINDER
};

struct Collider {
    ColliderType type;
    std::array<float, 3> size;

    Collider(const std::string& typeStr = "BOX",
             float x = 1.0f, float y = 1.0f, float z = 1.0f)
        : type(parseType(typeStr)), size{x, y, z} {}

    static ColliderType parseType(const std::string& str) {
        std::string upper = str;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

        if (upper == "BOX") return ColliderType::BOX;
        if (upper == "SPHERE") return ColliderType::SPHERE;
        if (upper == "CAPSULE") return ColliderType::CAPSULE;
        if (upper == "CYLINDER") return ColliderType::CYLINDER;

        Logger::Info("[Collider] Unknown type: " + str + ", defaulting to BOX");
        return ColliderType::BOX;
    }

    bool isValid() const {
        return size[0] > 0 && size[1] > 0 && size[2] > 0;
    }

    std::string getTypeString() const {
        switch (type) {
            case ColliderType::BOX: return "Box";
            case ColliderType::SPHERE: return "Sphere";
            case ColliderType::CAPSULE: return "Capsule";
            case ColliderType::CYLINDER: return "Cylinder";
            default: return "Unknown";
        }
    }
};

}
