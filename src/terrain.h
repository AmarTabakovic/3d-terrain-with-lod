#ifndef TERRAIN_H
#define TERRAIN_H

#include "heightmap.h"
#include "shader.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>

/**
 * @brief Abstract class containing shared members for all derived terrain
 *        algorithm classes.
 */
class Terrain {
private:

public:
    Terrain(std::string& heightmapFileName, std::string& textureFileName);
    ~Terrain() { }
    void loadHeightmap(std::string& fileName);
    void loadHeightmapImage(std::string& fileName);
    void loadHeightmapAsciiGrid(std::string& fileName);
    void loadTexture(std::string& fileName);
    Heightmap* heightmap;
    Shader* shader;
    unsigned int textureId;
    unsigned int terrainVAO, terrainVBO, terrainEBO;
    virtual void loadBuffers() = 0;
    virtual void unloadBuffers() = 0;
    virtual void render() = 0;
};

#endif // TERRAIN_H
