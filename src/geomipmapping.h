#ifndef GEOMIPMAPPING_H
#define GEOMIPMAPPING_H

#include "terrain.h"

/**
 * @brief The GeoMipMapping class
 */
class GeoMipMapping /*: Terrain*/ {
public:
    GeoMipMapping();
    void render();
    void loadBuffers();
    void unloadBuffers();
};

/**
 * @brief The Patch class
 */
class Patch {
public:
    float distance;
    unsigned int lod;
};

#endif // GEOMIPMAPPING_H
