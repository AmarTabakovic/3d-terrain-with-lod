#include "application.h"

#include "geomipmapping/geomipmapping.h"
#include "naiverenderer/naiverenderer.h"
#include "shader.h"
#include "skybox.h"

#include "stb_image.h"

#include <imgui.h>

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/gtc/matrix_transform.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

namespace Application {

/* Window variables */
const unsigned SCR_WIDTH = 1280;
const unsigned SCR_HEIGHT = 720;

unsigned windowWidth = SCR_WIDTH;
unsigned windowHeight = SCR_HEIGHT;

GLFWwindow* window;

/* FPS */
float deltaTime = 0.0f;
float lastFrame = 0.0f;

double fpsSum = 0.0f;
unsigned fpsCount = 0;

/* Main settings */
bool showOptions = true;
bool showMainOptions = true;
bool renderWireframe = false;
bool isDark = true;
bool renderSkybox = true;
bool doFog = true;
float fogDensity = 0.0003f;
float yScale = 1.0f / 30.0f;
float camYaw = 0.0f, camPitch = 0.0f, camRoll = 0.0f;
float camZoom;

/* Algorithms as strings for ImGui dropdown (I know this is somewhat hacky) */
const char* algos[] = { "NAIVE", "GEOMIPMAPPING" };
int selectedItemIndex = 1;

Camera camera = Camera(glm::vec3(0.0f, 1.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f),
    0.0f, 100000.0f, (float)windowWidth / (float)windowHeight,
    0.0f, -40.4f);

/* GeoMipMapping settings */
bool showGeoMipMappingOptions = true;
bool geoMipMappingDoubleDistEachLevel = false;
bool freezeCamera = false;
bool frustumCullingActive = true;
bool lodActive = true;
float geoMipMappingBaseDist = 700.0f;
unsigned geoMipMappingBlockSize = 257; /* Default block size, can be overwritten */
unsigned geoMipMappingMaxPossibleLods;
unsigned geoMipMappingMinLod = 0; /* Default, can be overwritten */
unsigned geoMipMappingMaxLod = 20; /* Default, can be overwritten */

/* Automatic camera movement settings */
bool showAutomaticMovementOptions = true;
float flightVel = 30; /* Default value */
float lookAroundVel = 7; /* Default value */
glm::vec3 camOrigin(0, 0, 0);
glm::vec3 camDest(0, 0, 0);

/* Terrain instances */
Terrain* naiveRenderer;
Terrain* geoMipMapping;
Terrain* current;
ActiveTerrain activeTerrain;

/* Skybox and colors */
Skybox* skybox;

glm::vec3 skyColor = glm::vec3(162.0f, 193.0f, 215.0f) * (1.0f / 255.0f);
glm::vec3 terrainColor = glm::vec3(120.0f, 117.0f, 115.0f) * (1.0f / 255.0f);

glm::vec3 lightDirection = camera.front();

/* Command line argument values */
std::string dataFolderPath;
std::string heightmapFileName;
std::string overlayFileName;
std::string skyboxFolderName = "simple-gradient"; /* Default skybox, can be overwritten */

bool loadGeoMipMapping = true; /* Load GeoMipMapping by default */
bool loadNaiveRendering = false; /* Do not load naive rendering by default */

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

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyboardInputCallback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    return 0;
}

void startAutomaticFlying()
{
    camera.origin = camOrigin;
    camera.destination = camDest;
    camera.direction = camDest - camOrigin;
    camera.isFlying = true;

    resetAverageFpsCounter();
}

void startAutomaticLookaround()
{
    camera.initialYaw = camera.yaw();
    camera.isLookingAround360 = true;

    resetAverageFpsCounter();
}

int parseArguments(int argc, char** argv)
{
    for (int i = 1; i < argc; i++) {
        std::string current = std::string(argv[i]);
        std::string property, value;
        size_t delimiterPos = current.find('=');

        if (delimiterPos != std::string::npos) {
            property = current.substr(0, delimiterPos);
            value = current.substr(delimiterPos + 1);

            if (property == "--block_size") {
                try {
                    geoMipMappingBlockSize = std::stoi(value);
                    geoMipMappingMaxPossibleLods = std::log2(geoMipMappingBlockSize - 1);
                } catch (std::invalid_argument const& ex) {
                    std::cout << "Block size must be an integer" << std::endl;
                }
            } else if (property == "--data_folder_path") {
                dataFolderPath = value;

            } else if (property == "--heightmap_file_name") {
                heightmapFileName = value;

            } else if (property == "--overlay_file_name") {
                overlayFileName = value;

            } else if (property == "--skybox_folder_name") {
                skyboxFolderName = value;

            } else if (property == "--naive_rendering") { /* Any input != 0 is true */
                loadNaiveRendering = value != "0";

            } else if (property == "--geomipmapping") { /* Any input != 0 is true */
                loadGeoMipMapping = value != "0";
            } else if (property == "--min_lod") {
                try {
                    geoMipMappingMinLod = std::stoi(value);
                } catch (std::invalid_argument const& ex) {
                    std::cout << "Min. LOD must be an integer" << std::endl;
                }

            } else if (property == "--max_lod") {
                try {
                    geoMipMappingMaxLod = std::stoi(value);
                } catch (std::invalid_argument const& ex) {
                    std::cout << "Max. LOD must be an integer" << std::endl;
                }

            } else {
                std::cerr << "Unknown option: " << property << std::endl;
                return 1;
            }
        } else { /* CLI option does not contain '=' sign */
            std::cerr << "Invalid input format" << std::endl;
            return 1;
        }
    }

    /* At least one terrain must be loaded */
    if (!loadGeoMipMapping && !loadNaiveRendering) {
        std::cerr << "Must load at least one terrain (naive or GeoMipMapping)" << std::endl;
        return 1;
    }

    /* At the very least, the folder name of the 'data' folder and the filename
     * of the heightmap are required to be specified by the user */
    if (dataFolderPath.empty()) {
        std::cerr << "The data folder path must be given as a command line argument" << std::endl;
        return 1;
    }
    if (heightmapFileName.empty()) {
        std::cerr << "The heightmap file name must be given as a command line argument" << std::endl;
        return 1;
    }

    return 0;
}

void renderMainOptions()
{
    float displayFps;
    if (deltaTime > 0.0f)
        displayFps = 1.0f / deltaTime;
    else
        displayFps = 0.0f;

    ImGui::Begin("Main options", &showMainOptions, ImGuiWindowFlags_MenuBar);
    ImGui::Text("Terrain size: %d x %d", current->width(), current->height());
    ImGui::Text("FPS: %d", (unsigned)displayFps);
    ImGui::SeparatorText("Terrain options");

    /* Note: changing the y-scale currently does not update the AABBs
     * of the GeoMipMappingBlocks, so bear this in mind when using this slider */
    ImGui::InputFloat("Y-Scale", &yScale, 0.1f, 2.0f, "%.2f");

    if (ImGui::BeginCombo("Current algorithm", algos[selectedItemIndex])) {
        for (int i = 0; i < IM_ARRAYSIZE(algos); i++) {
            const bool isSelected = (activeTerrain == static_cast<ActiveTerrain>(i));
            if (ImGui::Selectable(algos[i], isSelected)) {
                selectedItemIndex = i;
                activeTerrain = static_cast<ActiveTerrain>(i);
            }

            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    ImGui::SeparatorText("Camera options");
    ImGui::InputFloat("Camera yaw", &camYaw, 0.1f, 1.0f, "%.2f");
    ImGui::InputFloat("Camera pitch", &camPitch, 0.1f, 1.0f, "%.2f");
    ImGui::SliderFloat("Camera zoom", &camZoom, 1.0f, 90.0f, "%.2f");

    ImGui::SeparatorText("Shading options");

    if (!renderWireframe)
        ImGui::Checkbox("Render skybox", &renderSkybox);

    ImGui::InputFloat3("Lighting direction", (float*)&lightDirection, "%.2f");
    if (renderWireframe)
        ImGui::Checkbox("Dark theme", &isDark);
    else {
        ImGui::Checkbox("Distance fog", &doFog);
        if (doFog) {
            ImGui::InputFloat("Fog density", &fogDensity, 0.0001f, 0.0005f, "%.5f");
        }
    }

    ImGui::Checkbox("Wireframe mode", &renderWireframe);
    ImGui::End();
}

void renderAutomaticMovementOptions()
{
    ImGui::Begin("Automatic movement options", &showAutomaticMovementOptions, ImGuiWindowFlags_MenuBar);
    ImGui::SeparatorText("Flight");

    ImGui::SliderFloat("Flight velocity", &flightVel, 1, 100, "%.2f");
    ImGui::InputFloat3("Origin", (float*)&camOrigin, "%.2f");
    ImGui::InputFloat3("Destination", (float*)&camDest, "%.2f");

    if (ImGui::Button("Fly from origin to destination"))
        startAutomaticFlying();

    ImGui::SeparatorText("Look-around");

    ImGui::SliderFloat("Look-around velocity", &lookAroundVel, 1, 100, "%.2f");
    if (ImGui::Button("Look 360 degrees"))
        startAutomaticLookaround();

    ImGui::End();
}

void renderGeoMipMappingOptions()
{
    GeoMipMapping* casted = (GeoMipMapping*)current;
    unsigned nBlocksX = casted->nBlocksX();
    unsigned nBlocksZ = casted->nBlocksZ();

    ImGui::Begin("GeoMipMapping", &showGeoMipMappingOptions, ImGuiWindowFlags_MenuBar);
    ImGui::Text("Block size: %u", geoMipMappingBlockSize);
    ImGui::Text("Number of blocks on each axis: %d x %d", nBlocksX, nBlocksZ);
    ImGui::Text("Maximum number of possible LODs: %u", geoMipMappingMaxPossibleLods);
    ImGui::Text("User set number of LODs: %u", geoMipMappingMaxLod - geoMipMappingMinLod + 1);
    ImGui::Text("Minimum LOD: %u, maximum LOD: %u", geoMipMappingMinLod, geoMipMappingMaxLod);
    ImGui::InputFloat("Base distance", &geoMipMappingBaseDist, 100.0f, 1500.0f, "%.2f");
    ImGui::Checkbox("Double distance each level", &geoMipMappingDoubleDistEachLevel);
    ImGui::Checkbox("Culling active", &frustumCullingActive);
    ImGui::Checkbox("LOD active", &lodActive);
    ImGui::Checkbox("Freeze camera", &freezeCamera);
    ImGui::End();
}

void resetAverageFpsCounter()
{
    fpsSum = 0.0f;
    fpsCount = 0;
}

int run()
{
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

    /* Load skybox */
    skybox = new Skybox();
    skybox->loadBuffers();
    skybox->loadTexture(dataFolderPath + "/skybox/" + skyboxFolderName + "/");

    /* Load heightmap */
    Heightmap heightmap;
    heightmap.load(dataFolderPath + "/heightmaps/" + heightmapFileName, true);

    /* Set camera origin and destination to bottom left corner and top right corner respectively */
    camOrigin = glm::vec3(-(int)heightmap.width() / 2.0f, 200.0f, -(int)heightmap.height() / 2.0f);
    camDest = glm::vec3(heightmap.width() / 2.0f, 200.0f, heightmap.height() / 2.0f);

    /* Load naive rendering (if set in command line arguments) */
    if (loadNaiveRendering) {
        naiveRenderer = new NaiveRenderer(heightmap, 1.0f, yScale);
        naiveRenderer->loadBuffers();
        if (!overlayFileName.empty())
            naiveRenderer->loadTexture(dataFolderPath + std::string("/overlays/") + overlayFileName);

        current = naiveRenderer;
        activeTerrain = ActiveTerrain::NAIVE;
    }

    /* Load GeoMipMapping (if set in command line arguments) */
    if (loadGeoMipMapping) {
        geoMipMapping = new GeoMipMapping(heightmap, 1.0f, yScale, geoMipMappingBlockSize, geoMipMappingMinLod, geoMipMappingMaxLod);
        geoMipMapping->loadBuffers();

        if (!overlayFileName.empty())
            geoMipMapping->loadTexture(dataFolderPath + std::string("/overlays/") + overlayFileName);

        current = geoMipMapping;
        activeTerrain = ActiveTerrain::GEOMIPMAPPING;
    }

    /* Height values are now in vertices/textures, no longer needed in memory */
    heightmap.clear();

    float posLerp = 0.0f;
    float lookLerp = 0.0f;

    /* ============================= Main loop ============================= */
    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();

        /* ImGui */
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        /* Update frame time */
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        /* Add FPS to sum for average FPS calculation */
        fpsSum += (1.0f / deltaTime);
        fpsCount++;

        /* Update window title with framerate */
        std::string newTitle = "ATLOD: " + std::to_string((int)std::round(1.0f / deltaTime)) + " FPS";
        glfwSetWindowTitle(window, newTitle.c_str());

        /* Set active terrain */
        switch (activeTerrain) {
        case NAIVE:
            current = naiveRenderer;
            break;
        case GEOMIPMAPPING:
            current = geoMipMapping;
            break;
        }

        /* Get global camera yaw and pitch (for ImGui camera options) */
        camYaw = camera.yaw();
        camPitch = camera.pitch();
        camZoom = camera.zoom();

        if (showOptions) {
            renderMainOptions();
            renderAutomaticMovementOptions();

            if (activeTerrain == ActiveTerrain::GEOMIPMAPPING)
                renderGeoMipMappingOptions();
        }

        /* Overwrite camera yaw and pitch with values from ImGui options */
        camera.yaw(camYaw);
        camera.pitch(camPitch);
        camera.zoom(camZoom);
        camera.updateCameraVectors();

        /* Update camera values if flying */
        if (camera.isFlying) {
            camera.lerpFly(posLerp);
            posLerp += 0.0005 + flightVel / 50000;

            /* Finished flying */
            if (posLerp >= 1.0f) {
                camera.isFlying = false;
                posLerp = 0.0f;

                std::cout << "Average FPS of flight: " << fpsSum / fpsCount << std::endl;

                resetAverageFpsCounter();
            }
        }

        /* Update camera values if rotating 360 degrees */
        if (camera.isLookingAround360) {
            camera.lerpLook(lookLerp);
            lookLerp += lookAroundVel / 10000;

            /**/
            if (lookLerp >= 1.0f) {
                camera.isLookingAround360 = false;
                lookLerp = 0.0f;
                std::cout << "Average FPS of rotation: " << fpsSum / fpsCount << std::endl;

                resetAverageFpsCounter();
            }
        }

        processInput();

        camera.updateFrustum();

        /* Update window if resized */
        int wWidth, wHeight;
        glfwGetFramebufferSize(window, &wWidth, &wHeight);
        windowWidth = wWidth;
        windowHeight = wHeight;

        if (renderWireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            if (isDark)
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            else
                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.zoom()), (float)windowWidth / (float)windowHeight, 0.1f, 100000.0f);
        glm::mat4 view = camera.getViewMatrix();
        glm::vec2 settings = glm::vec2((float)isDark, (float)renderWireframe);
        current->shader().use();

        current->shader().setVec2("rendersettings", settings);
        current->shader().setMat4("projection", projection);
        current->shader().setMat4("view", view);
        current->shader().setVec3("cameraPos", camera.position());
        current->shader().setVec3("lightDirection", lightDirection);
        current->shader().setVec3("skyColor", skyColor);
        current->shader().setVec3("terrainColor", terrainColor);
        current->shader().setFloat("fogDensity", fogDensity);
        current->shader().setFloat("doFog", (float)doFog);

        /* Update GeoMipMapping options */
        if (activeTerrain == GEOMIPMAPPING) {
            GeoMipMapping* casted = (GeoMipMapping*)current;
            casted->baseDistance(geoMipMappingBaseDist);
            casted->doubleDistanceEachLevel(geoMipMappingDoubleDistEachLevel);
            casted->freezeCamera(freezeCamera);
            casted->lodActive(lodActive);
            casted->frustumCullingActive(frustumCullingActive);
            casted->yScale(yScale);
        }

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(current->xzScale(), current->yScale(), current->xzScale()));
        current->shader().setMat4("model", model);

        /* Naive terrain additionally requires normal matrix for shading */
        if (activeTerrain == NAIVE) {
            /* http://www.lighthouse3d.com/tutorials/glsl-12-tutorial/the-normal-matrix/ */
            current->shader().setMat4("normalMatrix", glm::transpose(glm::inverse(model)));
        }

        try {
            current->render(camera);
        } catch (std::exception e) {
            std::cout << "error: " << std::endl;
            std::cout << e.what() << std::endl;

            shutDown();
            return 1;
        }

        /* Render skybox */
        if (!renderWireframe && renderSkybox) {
            skybox->shader().use();
            view = glm::mat4(glm::mat3(camera.getViewMatrix()));
            skybox->shader().setMat4("projection", projection);
            skybox->shader().setMat4("view", view);
            skybox->render();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    shutDown();
    return 0;
}

void shutDown()
{
    /* Unload vertex and index buffers */
    skybox->unloadBuffers();

    if (loadGeoMipMapping)
        geoMipMapping->unloadBuffers();

    if (loadNaiveRendering)
        naiveRenderer->unloadBuffers();

    /* Delete instances */
    delete skybox;

    if (loadGeoMipMapping)
        delete geoMipMapping;
    if (loadNaiveRendering)
        delete naiveRenderer;

    glfwTerminate();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

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

void keyboardInputCallback(GLFWwindow* window, int key, int scanCode, int action, int modifiers)
{
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_O:
            showOptions = !showOptions;
            break;
        case GLFW_KEY_L: /* Set light direction to camera front vector */
            lightDirection = camera.front();
            break;
        case GLFW_KEY_R:
            startAutomaticLookaround();
            break;
        case GLFW_KEY_F:
            startAutomaticFlying();
            break;
        }
    }
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    camera.aspectRatio((float)width / (float)height);
}
}
