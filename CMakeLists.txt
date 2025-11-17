cmake_minimum_required(VERSION 3.10)
project(OpenGLRefactor)

set(OpenGL_GL_PREFERENCE "GLVND")

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(opengl_app main.cpp shader.cpp buffer.cpp input.cpp)

target_link_libraries(opengl_app
    PRIVATE
    OpenGL::GL
    GLUT::GLUT
    GLEW::GLEW
    glm::glm
)
