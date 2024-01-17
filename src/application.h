#ifndef APPLICATION_H
#define APPLICATION_H

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
 * @brief The ActiveTerrain enum
 */
enum ActiveTerrain {
    NAIVE = 0,
    GEOMIPMAPPING = 1
};

int setup();
int parseArguments(int argc, char** argv);
int run();
void shutDown();
void processInput();
void logFps();
void renderMainOptions();
void renderAutomaticMovementOptions();
void keyboardInputCallback(GLFWwindow* window, int key, int scanCode, int action, int modifiers);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);

};

#endif // APPLICATION_H
