#include <GL/glew.h>
#include <GL/glut.h>
#include "shader.h"
#include "buffer.h"
#include "input.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// This is the number of frames per second to render.
static const int FPS = 60;

// This global variable keeps track of the current orientation of the square.
static GLfloat currentAngleOfRotation = 0.0;

Shader* ourShader;
Buffer* ourBuffer;

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    ourShader->use();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(currentAngleOfRotation), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, -1.0f, 1.0f);

    ourShader->setMat4("model", model);
    ourShader->setMat4("projection", projection);

    ourBuffer->bind();
    glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
    ourBuffer->unbind();

    glutSwapBuffers();
}

void timer(int v) {
  if (Input::spinning) {
    currentAngleOfRotation += 1.0;
    if (currentAngleOfRotation > 360.0) {
      currentAngleOfRotation -= 360.0;
    }
    glutPostRedisplay();
  }
  glutTimerFunc(1000/FPS, timer, v);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowPosition(80, 80);
    glutInitWindowSize(800, 500);
    glutCreateWindow("Spinning Hexagon");

    glewInit();

    ourShader = new Shader("shader.vert", "shader.frag");

    float vertices[] = {
        // positions         // colors
        0.5f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // top right
        0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  // bottom left
        -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 0.0f,   // top left
        1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, // right
        -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f // left
    };

    unsigned int indices[] = {
        0, 1, 3,
        1, 2, 3,
        0, 1, 4,
        2, 3, 5
    };

    ourBuffer = new Buffer(vertices, sizeof(vertices), indices, sizeof(indices));

    glutDisplayFunc(display);
    glutTimerFunc(100, timer, 0);
    glutMouseFunc(Input::mouseCallback);
    glutMainLoop();

    delete ourShader;
    delete ourBuffer;

    return 0;
}
