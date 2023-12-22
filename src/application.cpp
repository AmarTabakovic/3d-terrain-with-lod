#include "application.h"

#include "geomipmapping/geomipmapping.h"
#include "naiverenderer/naiverenderer.h"
#include "shader.h"

#include "stb_image.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

namespace Application {

const unsigned SCR_WIDTH = 1000;
const unsigned SCR_HEIGHT = 800;

unsigned windowWidth = SCR_WIDTH;
unsigned windowHeight = SCR_HEIGHT;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

GLFWwindow* window;

bool renderWireframe = false;

ColorMode mode = DARK;

ActiveTerrain activeTerrain;

Terrain* naiveRenderer;
Terrain* geoMipMapping;

Terrain* current;

Camera camera = Camera(glm::vec3(-723.0f, 1050.5f, -723.9f),
    glm::vec3(0.0f, 1.0f, 0.0f),
    0.0f, 100000.0f, (float)windowWidth / (float)windowHeight,
    0.0f, -40.4f);

glm::vec3 skyColor = /*glm::vec3(92.0f, 160.0f, 255.0f)*/ glm::vec3(230.0f, 240.0f, 250.0f) * (1.0f / 255.0f);
glm::vec3 terrainColor = glm::vec3(120.0f, 117.0f, 115.0f) * (1.0f / 255.0f);

glm::vec3 lightPos(10000.0f, 5000.0f, 10000.0f);
glm::vec3 lightDirection = camera.front();

bool doFog = true;

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
        return 1;
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
        shutDown();
        return 1;
    }

    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // std::string heightmapPath = "../3d-terrain-with-lod/data/srtmgl-basel-30m.asc";
    // std::string texturePath = "../3d-terrain-with-lod/data/basel-texture-temp.png";

    // std::string heightmapPath = "../3d-terrain-with-lod/data/dom-0.5.xyz";
    // std::string texturePath = "../3d-terrain-with-lod/data/dom-texture-highres.png";

    std::string heightmapPath = "../3d-terrain-with-lod/data/alps-basel-srtm-heightmap-1.asc";
    //  std::string texturePath = "../3d-terrain-with-lod/data/alps-basel-srtm-relief-1.png";

    // std::string heightmapPath = "../3d-terrain-with-lod/data/alps-srtm-heightmap-2.asc";
    // std::string texturePath = "../3d-terrain-with-lod/data/alps-srtm-relief-2.png";

    // std::string heightmapPath = "../3d-terrain-with-lod/data/alps-srtm-heightmap-3.asc";
    //   std::string texturePath = "../3d-terrain-with-lod/data/alps-srtm-relief-3.png";

    //std::string heightmapPath = "../3d-terrain-with-lod/data/heightmap-srtm-30m-1.asc";
    //std::string texturePath = "../3d-terrain-with-lod/data/relief-srtm-30m-1.png";

    Heightmap heightmap;
    heightmap.load(heightmapPath);

    // naiveRenderer = new NaiveRenderer(heightmap, 1.0f, 1.0f / 30.0f);
    // naiveRenderer->loadBuffers();
    //    naiveRenderer->loadTexture(texturePath);
    //         naiveRenderer->shader->use();
    //         naiveRenderer->shader->setInt("texture1", 0);

    geoMipMapping = new GeoMipMapping(heightmap, 1.0f, 1.0f / 30.0f, 257); /* 65, 129, 257 work well*/
    geoMipMapping->loadBuffers();
    //  geoMipMapping->loadTexture(texturePath);
    //    geoMipMapping->shader->use();
    //    geoMipMapping->shader->setInt("texture1", 0);

    current = geoMipMapping;
    activeTerrain = ActiveTerrain::GEOMIPMAPPING;

    heightmap.clear();

    int fps = 0;
    float previousTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        fps++;

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        /* Log FPS */
        if (currentFrame - previousTime >= 1.0) {
            std::cout << "FPS: " << fps << std::endl;
            fps = 0;
            previousTime = currentFrame;
        }

        camera.updateFrustum();

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
        } else
            glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        current->shader().use();

        glm::mat4 projection = glm::perspective(glm::radians(camera.zoom()), (float)windowWidth / (float)windowHeight, 0.1f, 100000.0f);
        glm::mat4 view = camera.getViewMatrix();
        glm::vec2 settings = glm::vec2((float)mode, (float)renderWireframe);

        /* TODO: Set these uniform variables in the render() method of the
         * Terrain classes */
        current->shader().setVec2("rendersettings", settings);
        current->shader().setMat4("projection", projection);
        current->shader().setMat4("view", view);
        current->shader().setVec3("cameraPos", camera.position());
        current->shader().setVec3("lightDirection", lightDirection);
        current->shader().setVec3("skyColor", skyColor);
        current->shader().setVec3("terrainColor", terrainColor);
        current->shader().setFloat("doFog", (float)doFog);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(current->xzScale(), current->yScale(), current->xzScale()));
        current->shader().setMat4("model", model);

        /* http://www.lighthouse3d.com/tutorials/glsl-12-tutorial/the-normal-matrix/ */
        current->shader().setMat4("normalMatrix", glm::transpose(glm::inverse(model)));

        if (renderWireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        try {
            current->render(camera);
        } catch (std::exception e) {
            std::cout << "error: " << std::endl;
            std::cout << e.what() << std::endl;

            shutDown();
            return 1;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    shutDown();

    return 0;
}

void logFps()
{
}

/**
 * @brief shutDown
 */
void shutDown()
{
    geoMipMapping->unloadBuffers();
    //  naiveRenderer->unloadBuffers();

    glfwTerminate();
    delete geoMipMapping;
    //   delete naiveRenderer;

    std::cout << "ok";

    while (1)
        ;
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
        case GLFW_KEY_SPACE: /* Toggle wireframe */
            renderWireframe = !renderWireframe;
            break;
        case GLFW_KEY_M: /* Toggle light/dark mode*/
            mode = mode == DARK ? BRIGHT : DARK;
            break;
        case GLFW_KEY_F: /* Toggle fog */
            doFog = !doFog;
            break;
        case GLFW_KEY_L: /* Set light direction to camera front vector */
            lightDirection = camera.front();
            break;
        case GLFW_KEY_C: /* Freeze camera and frustum culling */
            camera.frozen = !camera.frozen;
            break;
        case GLFW_KEY_1: /* Switch to naive rendering */
            current = naiveRenderer;
            activeTerrain = NAIVE;
            break;
        case GLFW_KEY_2: /* Switch to GeoMipMapping */
            current = geoMipMapping;
            activeTerrain = GEOMIPMAPPING;
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
    /* TODO Resize camera frustum as well upon resize*/
    glViewport(0, 0, width, height);
    camera.aspectRatio((float)width / (float)height);
}
}
