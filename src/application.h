#ifndef APPLICATION_H
#define APPLICATION_H

#include "camera.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

/**
 *
 */
namespace Application {

/**
 * @brief The ColorMode enum
 */
enum ColorMode {
    DARK = 0,
    BRIGHT = 1
};

/**
 * @brief The CurrentTerrain enum
 */
enum CurrentTerrain {
    NAIVE = 0,
    GEOMIPMAPPING = 1
};

int setup();
int run();
void processInput();
void keyboardInputCallback(GLFWwindow* window, int key, int scanCode, int action, int modifiers);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);

};

#endif // APPLICATION_H
