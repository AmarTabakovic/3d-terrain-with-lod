#ifndef TERRAIN_H
#define TERRAIN_H

#include "camera.h"
#include "heightmap.h"
#include "shader.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>

/* For primitive restarts when creating index buffers */
const GLuint RESTART_INDEX = std::numeric_limits<GLuint>::max();

class Terrain {

public:
    Terrain();
    virtual ~Terrain() = 0;
    virtual void loadBuffers() = 0;
    virtual void unloadBuffers() = 0;
    virtual void render(Camera camera) = 0;

    void loadTexture(const std::string& fileName);
    void unloadTexture();

    /* Getters */
    Heightmap& heightmap();
    Shader& shader();
    unsigned width();
    unsigned height();
    float xzScale();
    float yScale();
    bool hasTexture();

    /* Setters */
    void yScale(float yScale);

protected:
    Heightmap _heightmap;
    Shader _shader;

    /* The width and height of the terrain can be different from
     * the heightmap dimensions */
    unsigned _width;
    unsigned _height;

    unsigned _textureId; /* OpenGL ID for the texture map */

    float _xzScale;
    float _yScale;

    bool _hasTexture = false;
};

#endif // TERRAIN_H
