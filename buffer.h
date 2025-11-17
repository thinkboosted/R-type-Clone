#ifndef BUFFER_H
#define BUFFER_H

#include <GL/glew.h>

class Buffer {
public:
    unsigned int VAO, VBO, EBO;

    Buffer(float* vertices, unsigned int vertSize, unsigned int* indices, unsigned int indSize);
    void bind();
    void unbind();
    ~Buffer();
};

#endif
