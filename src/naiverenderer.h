#ifndef NAIVERENDERER_H
#define NAIVERENDERER_H

#include "camera.h"
#include "shader.h"
#include "terrain.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "heightmap.h"

/**
 * @brief Encapsulates the naive terrain rendering algorithm in which
 *        each vertex gets rendered without any LOD considerations.
 */
class NaiveRenderer : public Terrain {
public:
    NaiveRenderer(std::string& heightmapFileName, std::string& textureFileName);
    ~NaiveRenderer();
    unsigned int terrainVAO, terrainVBO, terrainEBO;

    void loadBuffers();
    void render(Camera camera);
    void unloadBuffers();
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};

#endif // NAIVERENDERER_H
