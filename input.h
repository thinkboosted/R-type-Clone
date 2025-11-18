#ifndef INPUT_H
#define INPUT_H

#ifdef EMSCRIPTEN
#include <GLFW/glfw3.h>
#endif

class Input {
public:
    static bool spinning;
#ifdef EMSCRIPTEN
    static void glfwMouseCallback(GLFWwindow* window, int button, int action, int mods);
#else
    static void mouseCallback(int button, int state, int x, int y);
#endif
};

#endif
