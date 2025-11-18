#ifndef HEXAGON_APP_H
#define HEXAGON_APP_H

#include <memory>

class Shader;
class Buffer;

class HexagonApp {
public:
    HexagonApp();
    ~HexagonApp();

    void renderFrame();

private:
    void updateRotation();

    std::unique_ptr<Shader> shader_;
    std::unique_ptr<Buffer> buffer_;
    float currentAngle_ = 0.0f;
};

#endif // HEXAGON_APP_H
