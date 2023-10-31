#ifndef NAIVERENDERER_H
#define NAIVERENDERER_H

#include "shader.h"
// #include "terrain.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "heightmap.h"

/**
 * @brief Encapsulates the naive terrain rendering algorithm in which
 *        each vertex gets rendered without any LOD considerations.
 */
class NaiveRenderer /*: public Terrain*/ {
public:
    NaiveRenderer(std::string& fileName);
    ~NaiveRenderer();
    // void init();
    Shader* shader;
    void loadTexture(std::string& fileName);

    // Terrain(std::string& heightmapFile);
    void loadHeightmap(std::string& fileName);
    Heightmap* heightmap;

    unsigned int terrainVAO, terrainVBO, terrainEBO;
    unsigned int texture;

    void loadBuffers();

    // TODO: Add struct for rendering parameters
    void render();
    void unloadBuffers();
};

#endif // NAIVERENDERER_H
