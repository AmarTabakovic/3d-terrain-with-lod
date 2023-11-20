#include "naiverenderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/**
 * @brief NaiveRenderer::NaiveRenderer
 *
 * TODO: arguments for scale and shift? maybe for x and z grid spacing?
 */
NaiveRenderer::NaiveRenderer(std::string& heightmapFileName, std::string& textureFileName)
{
    shader = new Shader("../3d-terrain-with-lod/src/glsl/naiverenderer.vert", "../3d-terrain-with-lod/src/glsl/naiverenderer.frag");
    loadHeightmap(heightmapFileName);
    loadTexture(textureFileName);
    this->height = heightmap->height;
    this->width = heightmap->width;
}

/**
 * @brief NaiveRenderer::~NaiveRenderer
 */
NaiveRenderer::~NaiveRenderer()
{
    std::cout << "Naive terrain destroyed" << std::endl;
    delete shader;
    delete heightmap;
}

/**
 * @brief NaiveRenderer::render
 */
void NaiveRenderer::render(Camera camera)
{
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RESTART);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glBindVertexArray(terrainVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indices.size() * sizeof(unsigned int), GL_UNSIGNED_INT, (void*)0);

    GLenum error = glGetError();
    if (error != 0) {
        std::cout << "Error " << std::endl;
        std::exit(-1);
    }
}

/**
 * @brief Initializes and loads the buffers for rendering
 *
 * This method should be called once before entering the main rendering loop.
 */
void NaiveRenderer::loadBuffers()
{

    int signedWidth = (int)width;
    int signedHeight = (int)height;
    /* Set up vertex buffer */
    for (unsigned int i = 0; i < height; i++) {
        for (unsigned int j = 0; j < width; j++) {

            float y = heightmap->at(j, i);
            float x = (-signedWidth / 2.0f + signedWidth * j / (float)signedWidth);
            float z = (-signedHeight / 2.0f + signedHeight * i / (float)signedHeight);

            /* Render vertices around center point */
            vertices.push_back(x); /* vertex x */
            vertices.push_back(y); /* vertex y */
            vertices.push_back(z); /* vertex z */
            vertices.push_back((float)j / (float)signedWidth); /* texture x */
            vertices.push_back((float)i / (float)signedHeight); /* texture y */
        }
    }

    /* Set up index buffer */
    for (unsigned int i = 0; i < height - 1; i++) {
        for (unsigned int j = 0; j < width; j++) {
            indices.push_back(j + width * i);
            indices.push_back(j + width * (i + 1));
        }
        indices.push_back(RESTART);
    }

    glGenVertexArrays(1, &terrainVAO);
    glBindVertexArray(terrainVAO);

    glGenBuffers(1, &terrainVBO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    /* Position attribute */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    /* Texture attribute */
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &terrainEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    GLenum error = glGetError();
    if (error != 0) {
        std::cout << "Error " << std::endl;
    }
}

/**
 * @brief NaiveRenderer::unloadBuffers
 */
void NaiveRenderer::unloadBuffers()
{
    std::cout << "Unloading buffers" << std::endl;
    glDeleteVertexArrays(1, &terrainVAO);
    glDeleteBuffers(1, &terrainVBO);
    glDeleteBuffers(1, &terrainEBO);
}
