#include "terrain.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

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
        std::cout << "Loaded heightmap of size " << width << " x " << height << std::endl;
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
    std::ifstream heightmapFile(fileName);
    std::string line;
    unsigned int nCols, nRows;

    if (!heightmapFile.is_open()) {
        std::cout << "Error while reading heightmap file" << std::endl;
        std::exit(1);
    }

    try {
        /* First line */
        std::getline(heightmapFile, line);
        nCols = std::stoi(line.substr(line.find_first_of("0123456789")));

        /* Second line */
        std::getline(heightmapFile, line);
        nRows = std::stoi(line.substr(line.find_first_of("0123456789")));

        heightmap = new Heightmap(nCols, nRows);

        /* Skip rest until line 7 */
        for (int i = 3; i < 7; i++) {
            std::getline(heightmapFile, line);
            std::cout << line << std::endl;
        }

        while (std::getline(heightmapFile, line)) {
            std::string item;
            std::stringstream ss(line);

            /* Skip space at the beginning */
            std::getline(ss, item, ' ');

            while (std::getline(ss, item, ' ')) {
                heightmap->push(std::stoi(item));
            }
        }
    } catch (...) {
        std::cout << "Invalid .asc file" << std::endl;
        exit(1);
    }

    std::cout << "Loaded heightmap of size " << nCols << " x " << nRows << std::endl;

    heightmapFile.close();
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
