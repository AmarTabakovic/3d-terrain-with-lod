#ifndef SKYBOX_H
#define SKYBOX_H

#include "shader.h"

#include <string>
#include <vector>

/* Mainly based on learnopengl.com. */
class Skybox {
public:
    Skybox();
    void loadTexture(const std::string& fileName);
    void loadBuffers();
    void render();
    void unloadBuffers();
    Shader shader();

private:
    std::vector<unsigned> _textureFaces;
    Shader _shader;
    unsigned _textureId;
    unsigned _vbo;
    unsigned _vao;
};

#endif // SKYBOX_H
