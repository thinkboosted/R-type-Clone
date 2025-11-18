#include "platform.h"

#include "app.h"
#include "input.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#else
#include <GL/glew.h>
#include <GL/glut.h>
#endif

namespace {
HexagonApp* g_app = nullptr;

#ifdef EMSCRIPTEN
GLFWwindow* g_window = nullptr;

void frame() {
    if (!g_app) {
        return;
    }

    g_app->renderFrame();
    glfwSwapBuffers(g_window);
    glfwPollEvents();
}
#else
constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 500;
constexpr int kTimerIntervalMs = 1000 / 60;

void displayShim() {
    if (!g_app) {
        return;
    }

    g_app->renderFrame();
    glutSwapBuffers();
}

void timerShim(int value) {
    if (Input::spinning) {
        glutPostRedisplay();
    }

    glutTimerFunc(kTimerIntervalMs, timerShim, value);
}
#endif
} // namespace

namespace Platform {
void initialize(int argc, char** argv) {
#ifdef EMSCRIPTEN
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    g_window = glfwCreateWindow(800, 500, "Spinning Hexagon", nullptr, nullptr);
    glfwMakeContextCurrent(g_window);
#else
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowPosition(80, 80);
    glutInitWindowSize(kWindowWidth, kWindowHeight);
    glutCreateWindow("Spinning Hexagon");
    glewInit();
#endif
}

void run(HexagonApp& app) {
    g_app = &app;

#ifdef EMSCRIPTEN
    glfwSetMouseButtonCallback(g_window, Input::glfwMouseCallback);
    emscripten_set_main_loop(frame, 0, 1);
#else
    glutDisplayFunc(displayShim);
    glutTimerFunc(kTimerIntervalMs, timerShim, 0);
    glutMouseFunc(Input::mouseCallback);
    glutMainLoop();
#endif
}
} // namespace Platform
