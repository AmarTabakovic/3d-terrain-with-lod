#ifndef APPLICATION_H
#define APPLICATION_H

#include "camera.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace Application {
int setup();
int run();
void processInput();
void keyboardInputCallback(GLFWwindow* window, int key, int scanCode, int action, int modifiers);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);

};

#endif // APPLICATION_H
