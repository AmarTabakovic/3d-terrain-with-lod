#include "terrain.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "stb_image.h"

Terrain::Terrain()
{
}

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
}

void Terrain::unloadTexture()
{
    glDeleteTextures(1, &_textureId);
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
