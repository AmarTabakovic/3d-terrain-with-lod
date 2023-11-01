/**
 * TODO: Rewrite as namespace
 */

#include "application.h"

#include "naiverenderer.h"
#include "shader.h"

#include "stb_image.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

/**
 *
 */
namespace Application {

const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 800;

unsigned int windowWidth = SCR_WIDTH;
unsigned int windowHeight = SCR_HEIGHT;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

GLFWwindow* window;

bool renderWireframe = false;

ColorMode mode = DARK;

Camera camera = Camera(glm::vec3(123.0f, 250.5f, 123.9f),
    glm::vec3(0.0f, 1.0f, 0.0f),
    0.1f, -40.4f);

/**
 * @brief Application::setup
 * @return 0 on success, 1 otherwise
 */
int setup()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(windowWidth, windowHeight, "ATLOD", NULL, NULL);

    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    return 0;
}

/**
 * @brief Application::run
 * @return
 */
int run()
{
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyboardInputCallback);

    GLenum err = glewInit();

    if (err != GLEW_OK) {
        glfwTerminate();
        return 1;
    }

    glEnable(GL_DEPTH_TEST);

    // std::string path = "../3d-terrain-with-lod/data/dom-1028.png";
    std::string heightmapPath = "../3d-terrain-with-lod/data/basel-srtm-test.png";
    std::string texturePath = "../3d-terrain-with-lod/data/dom-texture-highres.png";
    NaiveRenderer naive(heightmapPath, texturePath);

    naive.loadBuffers();

    naive.shader->use();
    naive.shader->setInt("texture1", 0);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput();
        int wWidth, wHeight;
        glfwGetFramebufferSize(window, &wWidth, &wHeight);
        windowWidth = wWidth;
        windowHeight = wHeight;

        if (renderWireframe) {
            if (mode == DARK)
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            else
                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            glClearColor(89.0f / 255.0f, 153.0f / 255.0f, 1.0f, 1.0f);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        naive.shader->use();

        glm::mat4 projection = glm::perspective(glm::radians(camera.getZoom()), (float)windowWidth / (float)windowHeight, 0.1f, 100000.0f);
        glm::mat4 view = camera.getViewMatrix();
        glm::vec2 settings = glm::vec2((float)mode, (float)renderWireframe);

        naive.shader->setVec2("rendersettings", settings);

        naive.shader->setMat4("projection", projection);
        naive.shader->setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        naive.shader->setMat4("model", model);

        if (renderWireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        naive.render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    naive.unloadBuffers();
    glfwTerminate();
    return 0;
}

/**
 * @brief Application::processInput
 */
void processInput()
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::SPEED_UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::MOVE_FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::MOVE_BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::MOVE_LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::MOVE_RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::LOOK_UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::LOOK_DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::LOOK_LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.processKeyboard(CameraAction::LOOK_RIGHT, deltaTime);
}

/**
 * @brief keyboardInputCallback
 * @param window
 * @param key
 * @param scanCode
 * @param action
 * @param modifiers
 */
void keyboardInputCallback(GLFWwindow* window, int key, int scanCode, int action, int modifiers)
{
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_SPACE:
            renderWireframe = !renderWireframe;
            break;
        case GLFW_KEY_M:
            mode = mode == DARK ? BRIGHT : DARK;

            break;
        default:
            break;
        }
    }
}

/**
 * @brief Callback function for window resizes.
 *
 * @param window pointer to the window instance
 * @param width new window width
 * @param height new window height
 */
void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
}
