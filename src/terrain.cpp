#include "terrain.h"
#include <filesystem>
#include <iostream>

// #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/**
 * @brief Terrain::Terrain
 * @param heightmapFileName
 * @param textureFileName
 */
Terrain::Terrain(std::string& heightmapFileName, std::string& textureFileName)
{
}

/**
 * @brief Terrain::loadHeightmap
 * @param fileName
 */
void Terrain::loadHeightmap(std::string& fileName)
{
    const std::string& extension = std::filesystem::path(fileName).extension();
    if (extension == ".png" || extension == ".jpg") {
        loadHeightmapImage(fileName);
    } else if (extension == ".asc") {
        loadHeightmapAsciiGrid(fileName);
    } else {
        std::cout << "File extension not supported: " << extension << std::endl;
        std::exit(1);
    }
}

/**
 * @brief Terrain::loadHeightmapImage
 * @param fileName
 */
void Terrain::loadHeightmapImage(std::string& fileName)
{
    int width, height, nrChannels;

    unsigned char* data = stbi_load(fileName.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        std::cout << "Loaded heightmap of size " << height << " x " << width << std::endl;
    } else {
        std::cout << "Failed to load heightmap" << std::endl;
    }

    heightmap = new Heightmap(width, height);

    unsigned int bytePerPixel = nrChannels;

    for (unsigned int i = 0; i < height; i++) {
        for (unsigned int j = 0; j < width; j++) {
            unsigned char* pixelOffset = data + (j + width * i) * bytePerPixel;
            unsigned char y = pixelOffset[0];

            heightmap->push(y);
        }
    }

    stbi_image_free(data);
}

/**
 * @brief Terrain::loadHeightmapAsciiGrid
 *
 * TODO: Use a library for loading ASCII grids maybe?
 *
 * @param fileName
 */
void Terrain::loadHeightmapAsciiGrid(std::string& fileName)
{
    /*
     * 1. Load file with file name
     * 2. Iterate through each line from line 7 onwards
     * 3. Add coordinates
     */
    std::cout << "Not yet implemented" << std::endl;
    std::exit(1);
}

/**
 * @brief Terrain::loadTexture
 * @param fileName
 */
void Terrain::loadTexture(std::string& fileName)
{
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

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
