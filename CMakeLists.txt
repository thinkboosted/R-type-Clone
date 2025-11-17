cmake_minimum_required(VERSION 3.10)
project(OpenGLRefactor)

set(OpenGL_GL_PREFERENCE "GLVND")

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(GLEW REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLM REQUIRED glm)

add_executable(opengl_app main.cpp shader.cpp buffer.cpp input.cpp)

include_directories(${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS})

target_link_libraries(opengl_app ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} ${GLEW_LIBRARIES} ${GLM_LIBRARIES} glut)
