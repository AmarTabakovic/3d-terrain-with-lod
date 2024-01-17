#include "terrain.h"
#include <cstdio>
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
/*Terrain::Terrain(std::string& heightmapFileName, std::string& textureFileName)
{
}*/

Terrain::Terrain()
{
}

///**
// * @brief Terrain::loadHeightmap
// *
// * TODO: Add enum argument for limiting heightmap size, e,g,
// * - Default (no changes)
// * - PowerOfTwoPlusOne (2^n + 1)
// * - SquarePowerOfTwoPlusOne (square 2^n + 1)
// *
// * TODO: Refactor these methods to Heightmap class
// *
// * @param fileName
// */
// void Terrain::loadHeightmap(std::string& fileName)
//{
//    const std::string& extension = std::filesystem::path(fileName).extension();
//    if (extension == ".png" || extension == ".jpg") {
//        loadHeightmapImage(fileName);
//    } else if (extension == ".asc") {
//        loadHeightmapAsciiGrid(fileName);
//    } else if (extension == ".xyz") {
//        loadHeightmapXyz(fileName);
//    } else {
//        std::cerr << "File extension not supported: " << extension << std::endl;
//        std::exit(1);
//    }
//}

///**
// * @brief Load a heightmap with a simple list of XYZ points, as used by e.g. SwissTopo.
// * @param fileName
// */
// void Terrain::loadHeightmapXyz(std::string& fileName)
//{
//    std::ifstream heightmapFile(fileName);
//    std::string line;

//    if (!heightmapFile.is_open()) {
//        std::cerr << "Error while reading heightmap file" << std::endl;
//        std::exit(1);
//    }

//    /* Skip first line */
//    std::getline(heightmapFile, line);

//    unsigned nLines = 0;
//    while (std::getline(heightmapFile, line))
//        nLines++;

//    unsigned heightmapSize = std::sqrt(nLines);

//    if (heightmapSize * heightmapSize != nLines) {
//        std::cerr << "XYZ heightmap must be square" << std::endl;
//        std::exit(1);
//    }

//    heightmap = new Heightmap(heightmapSize, heightmapSize);

//    /* Reset file to beginning */
//    heightmapFile.clear();
//    heightmapFile.seekg(0);

//    /* Skip first line */
//    std::getline(heightmapFile, line);

//    while (std::getline(heightmapFile, line)) {
//        float x, y, z;
//        int ret = std::sscanf(line.c_str(), "%f %f %f",
//            &x, &z, &y);

//        if (ret != 3) {
//            std::cerr << "XYZ heightmap not formatted correctly" << std::endl;
//            std::exit(1);
//        }

//        heightmap->push(std::roundf(y));
//    }

//    std::cout << "Loaded heightmap of size " << heightmapSize << " x " << heightmapSize << std::endl;

//    heightmapFile.close();
//}

///**
// * @brief Terrain::loadHeightmapImage
// * @param fileName
// */
// void Terrain::loadHeightmapImage(std::string& fileName)
//{
//    int width, height, nrChannels;

//    unsigned char* data = stbi_load(fileName.c_str(), &width, &height, &nrChannels, 0);

//    if (data) {
//        std::cout << "Loaded heightmap of size " << width << " x " << height << std::endl;
//    } else {
//        std::cerr << "Failed to load heightmap" << std::endl;
//        std::exit(1);
//    }

//    heightmap = new Heightmap(width, height);

//    unsigned bytePerPixel = nrChannels;

//    for (unsigned i = 0; i < height; i++) {
//        for (unsigned j = 0; j < width; j++) {
//            unsigned char* pixelOffset = data + (j + width * i) * bytePerPixel;
//            unsigned char y = pixelOffset[0];

//            heightmap->push(y);
//        }
//    }

//    stbi_image_free(data);
//}

///**
// * @brief Terrain::loadHeightmapAsciiGrid
// *
// * TODO: Use a library for loading ASCII grids maybe?
// *
// * @param fileName
// */
// void Terrain::loadHeightmapAsciiGrid(std::string& fileName)
//{
//    std::ifstream heightmapFile(fileName);
//    std::string line;
//    unsigned nCols, nRows;

//    if (!heightmapFile.is_open()) {
//        std::cerr << "Error while reading heightmap file" << std::endl;
//        std::exit(1);
//    }

//    try {
//        /* First line */
//        std::getline(heightmapFile, line);
//        nCols = std::stoi(line.substr(line.find_first_of("0123456789")));

//        /* Second line */
//        std::getline(heightmapFile, line);
//        nRows = std::stoi(line.substr(line.find_first_of("0123456789")));

//        heightmap = new Heightmap(nCols, nRows);

//        /* Skip rest until line 7 */
//        for (int i = 3; i < 7; i++) {
//            std::getline(heightmapFile, line);
//            std::cout << line << std::endl;
//        }

//        while (std::getline(heightmapFile, line)) {
//            std::string item;
//            std::stringstream ss(line);

//            /* Skip space at the beginning */
//            std::getline(ss, item, ' ');

//            while (std::getline(ss, item, ' ')) {
//                heightmap->push(std::stoi(item));
//            }
//        }
//    } catch (...) {
//        std::cerr << "Invalid .asc file" << std::endl;
//        std::exit(1);
//    }

//    std::cout << "Loaded heightmap of size " << nCols << " x " << nRows << std::endl;

//    heightmapFile.close();
//}

/**
 * @brief Terrain::loadTexture
 * @param fileName
 */
void Terrain::loadTexture(const std::string& fileName)
{
    std::cout << "Loading texture" << std::endl;
    glGenTextures(1, &_textureId);
    glBindTexture(GL_TEXTURE_2D, _textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(fileName.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        // TODO: RGBA?
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Failed to load texture" << std::endl;
        std::exit(1);
    }
    stbi_image_free(data);

    std::cout << "Loaded texture" << std::endl;
    _hasTexture = true;
    shader().use();
    shader().setInt("texture1", 0);

    /* Bind texture ID */
    /*shader->use();
    shader->setInt("texture1", 0);*/
}

unsigned Terrain::width()
{
    return _width;
}

unsigned Terrain::height()
{
    return _height;
}

float Terrain::xzScale()
{
    return _xzScale;
}

float Terrain::yScale()
{
    return _yScale;
}

void Terrain::yScale(float yScale)
{
    _yScale = yScale;
}

Shader& Terrain::shader()
{
    return _shader;
}

Terrain::~Terrain()
{
}
