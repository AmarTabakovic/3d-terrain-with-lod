#include "heightmap.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "atlodutil.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "stb_image.h"

/**
 * @brief Heightmap::Heightmap
 * @param width
 * @param height
 */
/*Heightmap::Heightmap(unsigned width, unsigned height)
    : _width(width)
    , _height(height)
{
}*/

Heightmap::Heightmap()
{
}

unsigned Heightmap::width()
{
    return _width;
}

unsigned Heightmap::height()
{
    return _height;
}

std::vector<unsigned short> Heightmap::data()
{
    return _data;
}

/**
 * @brief TODO: Is this necessary? I could also define heightmap as pointer and delete
 */
void Heightmap::clear()
{
    _data.clear();
    _data.shrink_to_fit();
    _height = 0;
    _width = 0;
}

void Heightmap::generateGlTexture() {

//    float maxHm = -9999999, minHm = 9999999;
//    std::vector<float> hm2;
//    for (auto s : _data) {
//        maxHm = std::max(maxHm, (float)s);
//        minHm = std::min(minHm, (float)s);
//        max = maxHm;
//        min = minHm;
//    }
//    for (auto s : _data) {
//        float newVal = ((float)s - minHm) / (maxHm - minHm);
//        hm2.push_back(newVal);
//    }

    glGenTextures(1, &_heightmapTextureId);
    glBindTexture(GL_TEXTURE_2D, _heightmapTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, _width, _height, 0, GL_RED, GL_UNSIGNED_SHORT, tempData);
    AtlodUtil::checkGlError("Heightmap texture loading failed");
    glBindTexture(GL_TEXTURE_2D, 0);

//    unsigned bytePerPixel = 1 * 2;

//    max = 0, min = 60000;
//    for (unsigned i = 0; i < _height; i++) {
//        for (unsigned j = 0; j < _width; j++) {
//            unsigned short* pixelOffset = tempData + (j + _width * i) * bytePerPixel;
//            unsigned short y = pixelOffset[0];

//            max = std::max(max, y);
//            min = std::min(min, y);

//        }
//    }
}

unsigned Heightmap::heightmapTextureId() {
    return _heightmapTextureId;
}

/**
 * @brief Heightmap::load
 *
 * TODO: Add enum argument for limiting heightmap size, e,g,
 * - Default (no changes)
 * - PowerOfTwoPlusOne (2^n + 1)
 * - SquarePowerOfTwoPlusOne (square 2^n + 1)
 *
 * TODO: Refactor these methods to Heightmap class
 *
 * @param fileName
 */
void Heightmap::load(const std::string& fileName)
{
    const std::string& extension = std::filesystem::path(fileName).extension();
    if (extension == ".png") {
        loadImage(fileName);
    }
//    else if (extension == ".asc") {
//        loadAsciiGrid(fileName);
//    } else if (extension == ".xyz") {
//        loadXyz(fileName);
//    }
    else {
        std::cerr << "File extension not supported: " << extension << std::endl;
        std::exit(1);
    }
}

///**
// * @brief Load a heightmap with a simple list of XYZ points, as used by e.g. SwissTopo.
// * @param fileName
// */
//void Heightmap::loadXyz(std::string& fileName)
//{
//    std::ifstream heightmapFile(fileName);
//    std::string line;

//    if (!heightmapFile.is_open()) {
//        std::cerr << "Error while reading heightmap file" << std::endl;
//        std::exit(1);
//    }

//    /* Skip first line */
//    std::getline(heightmapFile, line);

//    /* Count number of lines to determine dimension of heightmap */
//    unsigned nLines = 0;
//    while (std::getline(heightmapFile, line))
//        nLines++;

//    /* XYZ heightmaps must be square */
//    unsigned heightmapSize = std::sqrt(nLines);

//    if (heightmapSize * heightmapSize != nLines) {
//        std::cerr << "XYZ heightmap must be square" << std::endl;
//        std::exit(1);
//    }

//    _width = heightmapSize;
//    _height = heightmapSize;

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

//        push(std::roundf(y));
//    }

//    std::cout << "Loaded heightmap of size " << heightmapSize << " x " << heightmapSize << std::endl;

//    heightmapFile.close();
//}

/**
 * @brief Heightmap::loadHeightmapImage
 * @param fileName
 */
void Heightmap::loadImage(const std::string& fileName)
{
    int width, height, nrChannels;

    //unsigned char* data = stbi_load(fileName.c_str(), &width, &height, &nrChannels, 0);
    unsigned short* data = stbi_load_16(fileName.c_str(), &width, &height, &nrChannels, STBI_grey);
    tempData = stbi_load_16(fileName.c_str(), &width, &height, &nrChannels, STBI_grey);

    if (data) {
        std::cout << "Loaded heightmap image of size " << width << " x " << height << std::endl;
        std::cout << "Num channels: " << nrChannels << std::endl;
    } else {
        std::cerr << "Failed to load heightmap" << std::endl;
        std::exit(1);
    }

    this->_width = width;
    this->_height = height;

    unsigned bytePerPixel = nrChannels;

    for (unsigned i = 0; i < height; i++) {
        for (unsigned j = 0; j < width; j++) {
            unsigned short* pixelOffset = data + (j + width * i) * bytePerPixel;
            unsigned short y = pixelOffset[0];

            push(y);
        }
    }

    stbi_image_free(data);
}

///**
// * @brief Terrain::loadHeightmapAsciiGrid
// *
// * TODO: Use a library for loading ASCII grids maybe?
// *
// * @param fileName
// */
//void Heightmap::loadAsciiGrid(std::string& fileName)
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

//        // heightmap = new Heightmap(nCols, nRows);
//        _width = nCols;
//        _height = nRows;

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
//                push(std::stoi(item));
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
 * @brief Heightmap::at
 * @param x
 * @param y
 * @return
 */
unsigned Heightmap::at(unsigned x, unsigned z)
{
    /* z -> row, x -> column*/
    try {
        return _data.at(z * _width + x);
    } catch (std::out_of_range) {
        std::cout << "Failed fetching height at " << z << ", " << x << std::endl;
        std::exit(1);
    }
}

void Heightmap::push(unsigned heightValue)
{
    _data.push_back(heightValue);
}
