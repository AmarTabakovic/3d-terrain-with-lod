#include "heightmap.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

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

/**
 * @brief generate
 *
 * TODO: Perlin noise? Simplex noise? Midplace displacement?
 */
void Heightmap::generate()
{
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
void Heightmap::load(std::string& fileName)
{
    const std::string& extension = std::filesystem::path(fileName).extension();
    if (extension == ".png" || extension == ".jpg") {
        loadImage(fileName);
    } else if (extension == ".asc") {
        loadAsciiGrid(fileName);
    } else if (extension == ".xyz") {
        loadXyz(fileName);
    } else {
        std::cerr << "File extension not supported: " << extension << std::endl;
        std::exit(1);
    }
}

/**
 * @brief Load a heightmap with a simple list of XYZ points, as used by e.g. SwissTopo.
 * @param fileName
 */
void Heightmap::loadXyz(std::string& fileName)
{
    std::ifstream heightmapFile(fileName);
    std::string line;

    if (!heightmapFile.is_open()) {
        std::cerr << "Error while reading heightmap file" << std::endl;
        std::exit(1);
    }

    /* Skip first line */
    std::getline(heightmapFile, line);

    /* Count number of lines to determine dimension of heightmap */
    unsigned nLines = 0;
    while (std::getline(heightmapFile, line))
        nLines++;

    /* XYZ heightmaps must be square */
    unsigned heightmapSize = std::sqrt(nLines);

    if (heightmapSize * heightmapSize != nLines) {
        std::cerr << "XYZ heightmap must be square" << std::endl;
        std::exit(1);
    }

    _width = heightmapSize;
    _height = heightmapSize;

    /* Reset file to beginning */
    heightmapFile.clear();
    heightmapFile.seekg(0);

    /* Skip first line */
    std::getline(heightmapFile, line);

    while (std::getline(heightmapFile, line)) {
        float x, y, z;
        int ret = std::sscanf(line.c_str(), "%f %f %f",
            &x, &z, &y);

        if (ret != 3) {
            std::cerr << "XYZ heightmap not formatted correctly" << std::endl;
            std::exit(1);
        }

        push(std::roundf(y));
    }

    std::cout << "Loaded heightmap of size " << heightmapSize << " x " << heightmapSize << std::endl;

    heightmapFile.close();
}

/**
 * @brief Terrain::loadHeightmapImage
 * @param fileName
 */
void Heightmap::loadImage(std::string& fileName)
{
    int width, height, nrChannels;

    unsigned char* data = stbi_load(fileName.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        std::cout << "Loaded heightmap of size " << width << " x " << height << std::endl;
    } else {
        std::cerr << "Failed to load heightmap" << std::endl;
        std::exit(1);
    }

    this->_width = width;
    this->_height = height;
    // heightmap = new Heightmap(width, height);

    unsigned bytePerPixel = nrChannels;

    for (unsigned i = 0; i < height; i++) {
        for (unsigned j = 0; j < width; j++) {
            unsigned char* pixelOffset = data + (j + width * i) * bytePerPixel;
            unsigned char y = pixelOffset[0];

            push(y);
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
void Heightmap::loadAsciiGrid(std::string& fileName)
{
    std::ifstream heightmapFile(fileName);
    std::string line;
    unsigned nCols, nRows;

    if (!heightmapFile.is_open()) {
        std::cerr << "Error while reading heightmap file" << std::endl;
        std::exit(1);
    }

    try {
        /* First line */
        std::getline(heightmapFile, line);
        nCols = std::stoi(line.substr(line.find_first_of("0123456789")));

        /* Second line */
        std::getline(heightmapFile, line);
        nRows = std::stoi(line.substr(line.find_first_of("0123456789")));

        // heightmap = new Heightmap(nCols, nRows);
        _width = nCols;
        _height = nRows;

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
                push(std::stoi(item));
            }
        }
    } catch (...) {
        std::cerr << "Invalid .asc file" << std::endl;
        std::exit(1);
    }

    std::cout << "Loaded heightmap of size " << nCols << " x " << nRows << std::endl;

    heightmapFile.close();
}

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
