#ifndef TERRAIN_H
#define TERRAIN_H

#include "camera.h"
#include "heightmap.h"
#include "shader.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>

const GLuint RESTART_INDEX = std::numeric_limits<GLuint>::max();

class RenderArgs {
    /* TODO: Stuff like
     *  - wireframeOn
     *  - wireframeColorOn
     *  - fogOn
     *  - lightingOn
     *  - darkModeOn
     *  - frustumCullingOn (only if supported, e.g. GeoMipMapping)
     *  - textureOn
     */
};

/**
 * @brief Abstract class containing shared members for all derived terrain
 *        algorithm classes.
 */
class Terrain {
private:

public:
    Terrain();
    virtual ~Terrain() = 0;
    virtual void loadBuffers() = 0;
    virtual void unloadBuffers() = 0;
    virtual void render(Camera camera) = 0;

    void loadTexture(const std::string& fileName);

    Heightmap& heightmap();
    Shader& shader();

    unsigned width();
    unsigned height();
    float xzScale();
    float yScale();

    void yScale(float yScale);

    bool hasTexture();
    bool hasNormal();
    bool hasDetail();

protected:
    /* TODO: these two can probably be non-pointers */
    Heightmap _heightmap;
    Shader _shader;

    /* The width and height of the terrain can be different from
     * the heightmap dimensions */
    unsigned _width;
    unsigned _height;

    unsigned _textureId; /* OpenGL ID for the texture map */
    unsigned _normalId; /* OpenGL ID for the normal map */
    unsigned _detailId; /* OpenGL ID for the detail map */

    float _xzScale; /* TODO: Unneeded? */
    float _yScale;

    bool _hasTexture = false;
    bool _hasNormal = false;
    bool _hasDetail = false;
};

#endif // TERRAIN_H
