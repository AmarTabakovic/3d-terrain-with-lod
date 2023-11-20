#ifndef TERRAIN_H
#define TERRAIN_H

#include "camera.h"
#include "heightmap.h"
#include "shader.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>

const GLuint RESTART = std::numeric_limits<GLuint>::max();

/**
 * @brief Abstract class containing shared members for all derived terrain
 *        algorithm classes.
 */
class Terrain {
private:

public:
    // Terrain(std::string& heightmapFileName, std::string& textureFileName);
    Terrain();
    virtual ~Terrain() = 0;
    void loadHeightmap(std::string& fileName);
    void loadHeightmapImage(std::string& fileName);
    void loadHeightmapAsciiGrid(std::string& fileName);
    void loadHeightmapXyz(std::string& fileName);
    void loadTexture(std::string& fileName);
    // TODO these two can probably be non-pointers
    Heightmap* heightmap;
    Shader* shader;
    unsigned int width;
    unsigned int height;
    unsigned int textureId;
    unsigned int terrainVAO, terrainVBO, terrainEBO;
    virtual void loadBuffers() = 0;
    virtual void unloadBuffers() = 0;
    virtual void render(Camera camera) = 0;
    float xzScale = 1;
    float yScale = 1.0f / 30.0f;
};

#endif // TERRAIN_H
