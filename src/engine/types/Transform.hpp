#pragma once

#include <cmath>

namespace rtypeEngine {

struct Transform {
    float x, y, z;        // Position
    float rx, ry, rz;     // Rotation (Euler angles in radians)
    float sx, sy, sz;     // Scale

    Transform(float x = 0, float y = 0, float z = 0,
              float rx = 0, float ry = 0, float rz = 0,
              float sx = 1, float sy = 1, float sz = 1)
        : x(x), y(y), z(z), rx(rx), ry(ry), rz(rz), sx(sx), sy(sy), sz(sz) {}

    void setPosition(float x, float y, float z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    void setRotation(float rx, float ry, float rz) {
        this->rx = rx;
        this->ry = ry;
        this->rz = rz;
    }

    void setScale(float s) {
        this->sx = s;
        this->sy = s;
        this->sz = s;
    }

    float getDistance(const Transform& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        float dz = z - other.z;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }
};

}
