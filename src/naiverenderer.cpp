#include "naiverenderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/**
 * @brief NaiveRenderer::NaiveRenderer
 *
 * TODO: arguments for scale and shift? maybe for x and z grid spacing?
 */
NaiveRenderer::NaiveRenderer(std::string& fileName)
//: Terrain(fileName)
{
    shader = new Shader("../3d-terrain-with-lod/src/glsl/naiverenderer.vert", "../3d-terrain-with-lod/src/glsl/naiverenderer.frag");
    loadHeightmap(fileName);

    std::string textureFile = "../3d-terrain-with-lod/data/dom-texture-highres.png";
    loadTexture(textureFile);
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
 * @brief NaiveRenderer::loadTexture
 * @param fileName
 */
void NaiveRenderer::loadTexture(std::string& fileName)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(fileName.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
}

/**
 * @brief NaiveRenderer::loadHeightmap
 * @param fileName
 *
 * TODO: GDAL for GeoTIFF?
 */
void NaiveRenderer::loadHeightmap(std::string& fileName)
{
    int width, height, nrChannels;

    unsigned char* data = stbi_load(fileName.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        std::cout << "Loaded heightmap of size " << height << " x " << width << std::endl;
    } else {
        std::cout << "Failed to load texture" << std::endl;
    }

    heightmap = new Heightmap(width, height);

    unsigned int bytePerPixel = nrChannels;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            unsigned char* pixelOffset = data + (j + width * i) * bytePerPixel;
            unsigned char y = pixelOffset[0];

            heightmap->push(y);
        }
    }

    stbi_image_free(data);
}

/**
 * @brief render
 */
void NaiveRenderer::render()
{
    int height = heightmap->height;
    int width = heightmap->width;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindVertexArray(terrainVAO);

    for (unsigned int strip = 0; strip < height - 1; strip++) {
        glDrawElements(GL_TRIANGLE_STRIP,
            width * 2,
            GL_UNSIGNED_INT,
            (void*)(sizeof(unsigned int) * (width * 2) * strip));
    }
}

/**
 * @brief NaiveRenderer::loadBuffers
 */
void NaiveRenderer::loadBuffers()
{
    unsigned int height = heightmap->height;
    unsigned int width = heightmap->width;
    std::vector<float> vertices;

    float scale = 1.2f; /* Scale in y direction */

    for (unsigned int i = 0; i < height; i++) {
        for (unsigned int j = 0; j < width; j++) {

            unsigned char y = heightmap->at(j, i);

            /* Render vertices around center point */
            vertices.push_back(-height / 2.0f + height * i / (float)height); /* vertex x */
            vertices.push_back((float)y * scale); /* vertex y */
            vertices.push_back(-width / 2.0f + width * j / (float)width); /* vertex z */
            vertices.push_back((float)j / (float)width); /* texture x */
            vertices.push_back((float)i / (float)height); /* texture y */
        }
    }

    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < height - 1; i++) {
        for (unsigned int j = 0; j < width; j++) {
            for (unsigned int k = 0; k < 2; k++) {
                indices.push_back(j + width * (i + k));
            }
        }
    }

    glGenVertexArrays(1, &terrainVAO);
    glBindVertexArray(terrainVAO);

    glGenBuffers(1, &terrainVBO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &terrainEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    std::cout << "LOAD ERROR" << glGetError() << std::endl;
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
