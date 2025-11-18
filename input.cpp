#include "input.h"

#ifdef EMSCRIPTEN
#include <GLFW/glfw3.h>
#else
#include <GL/glut.h>
#endif

bool Input::spinning = true;

#ifdef EMSCRIPTEN
void Input::glfwMouseCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        spinning = true;
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        spinning = false;
    }
}
#else
void Input::mouseCallback(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        spinning = true;
    } else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        spinning = false;
    }
}
#endif
