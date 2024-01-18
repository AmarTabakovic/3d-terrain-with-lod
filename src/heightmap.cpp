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

void Heightmap::clear()
{
    _data.clear();
    _data.shrink_to_fit();
    _height = 0;
    _width = 0;
}

void Heightmap::generateGlTexture(unsigned short* data)
{

    glGenTextures(1, &_heightmapTextureId);
    glBindTexture(GL_TEXTURE_2D, _heightmapTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, _width, _height, 0, GL_RED, GL_UNSIGNED_SHORT, data);
    AtlodUtil::checkGlError("Heightmap texture loading failed");
    glBindTexture(GL_TEXTURE_2D, 0);
}

unsigned Heightmap::heightmapTextureId() {
    return _heightmapTextureId;
}

void Heightmap::load(const std::string& fileName, bool loadTextureHeightmap)
{
    const std::string& extension = std::filesystem::path(fileName).extension();
    if (extension == ".png") {
        loadImage(fileName, loadTextureHeightmap);
    }
    else {
        std::cerr << "File extension not supported: " << extension << std::endl;
        std::exit(1);
    }
}

void Heightmap::loadImage(const std::string& fileName, bool loadTextureHeightmap)
{
    int width, height, nrChannels;
    unsigned short* data = stbi_load_16(fileName.c_str(), &width, &height, &nrChannels, STBI_grey);

    if (data) {

        _width = width;
        _height = height;

        if (loadTextureHeightmap)
            generateGlTexture(data);

        std::cout << "Loaded heightmap image of size " << width << " x " << height << std::endl;
        std::cout << "Num channels: " << nrChannels << std::endl;
    } else {
        std::cerr << "Failed to load heightmap" << std::endl;
        std::exit(1);
    }

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
