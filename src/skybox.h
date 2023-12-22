#ifndef SKYBOX_H
#define SKYBOX_H

#include "shader.h"

#include <string>

/**
 * @brief The Skybox class
 */
class Skybox {
public:
    Skybox();
    void loadTexture(std::string& fileName);
    void loadBuffers();
    void render();
    void unloadBuffers();

private:
    Shader _shader;
    unsigned _textureId;
    unsigned _ebo;
    unsigned _vbo;
    unsigned _vao;
};

#endif // SKYBOX_H
