cmake_minimum_required(VERSION 3.10)
project(OpenGL)

set(OpenGL_GL_PREFERENCE "GLVND")

include(FetchContent)

# Fetch and make available GLFW for Emscripten build
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.8 # Use a specific stable tag
)

find_package(OpenGL REQUIRED)

# Prefer fetching glm at configure time so we don't have to vendor it in Git.
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        0.9.9.8
)

FetchContent_MakeAvailable(glm)

add_executable(opengl_app main.cpp shader.cpp buffer.cpp input.cpp app.cpp platform.cpp)
target_link_libraries(opengl_app PRIVATE glm)

if(EMSCRIPTEN)
    target_link_options(opengl_app PRIVATE "-sUSE_GLFW=3" "-sFULL_ES3=1" "--shell-file=${CMAKE_SOURCE_DIR}/shell.html")
    # Rename the output to .html
    set_target_properties(opengl_app PROPERTIES SUFFIX ".html")
else()
    FetchContent_MakeAvailable(glfw)
    find_package(GLUT REQUIRED)
    find_package(GLEW REQUIRED)

    # Link native libraries
    target_link_libraries(opengl_app
        PRIVATE
        OpenGL::GL
        GLUT::GLUT
        GLEW::GLEW
        glfw
    )
endif()
