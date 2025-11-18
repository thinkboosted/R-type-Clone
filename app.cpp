#include "app.h"

#include "shader.h"
#include "buffer.h"
#include "input.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>

namespace {
constexpr float kRotationIncrement = 1.0f;
constexpr float kFullRotation = 360.0f;

float vertices[] = {
    // positions         // colors
     0.5f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // top right
     0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  // bottom right
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  // bottom left
    -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 0.0f,  // top left
     1.0f,  0.0f, 0.0f,  0.0f, 1.0f, 1.0f,  // right
    -1.0f,  0.0f, 0.0f,  1.0f, 0.0f, 1.0f   // left
};

unsigned int indices[] = {
    0, 1, 3,
    1, 2, 3,
    0, 1, 4,
    2, 3, 5
};
}

HexagonApp::HexagonApp() {
    std::filesystem::path currentPath = __FILE__;
    std::filesystem::path shaderDir = currentPath.parent_path();
    std::string vertexShaderPath = (shaderDir / "shader.vert").string();
    std::string fragmentShaderPath = (shaderDir / "shader.frag").string();

    shader_ = std::make_unique<Shader>(vertexShaderPath.c_str(), fragmentShaderPath.c_str());
    buffer_ = std::make_unique<Buffer>(vertices, sizeof(vertices), indices, sizeof(indices));
}

HexagonApp::~HexagonApp() = default;

void HexagonApp::renderFrame() {
    updateRotation();

    glClear(GL_COLOR_BUFFER_BIT);

    shader_->use();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(currentAngle_), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, -1.0f, 1.0f);

    shader_->setMat4("model", model);
    shader_->setMat4("projection", projection);

    buffer_->bind();
    glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
    buffer_->unbind();
}

void HexagonApp::updateRotation() {
    if (!Input::spinning) {
        return;
    }

    currentAngle_ += kRotationIncrement;
    if (currentAngle_ > kFullRotation) {
        currentAngle_ -= kFullRotation;
    }
}
