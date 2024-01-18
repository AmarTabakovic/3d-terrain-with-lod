#ifndef GEOMIPMAPPINGBLOCK_H
#define GEOMIPMAPPINGBLOCK_H

// #include "geomipmapping.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

/**
 * Forward declaration
 */
class GeoMipMapping;

/**
 * @brief The GeoMipMappingPatch class
 *
 * TODO: Add method for pushing quads using two indices and step.
 */
class GeoMipMappingBlock {
public:
    GeoMipMappingBlock(unsigned blockId, glm::vec3 trueCenter, glm::vec3 aabbCenter, /* float minY, float maxY*/ glm::vec3 p1, glm::vec3 p2, glm::vec2 translation);

    /* Pointer to the current GeoMipMapping instance */
    GeoMipMapping* terrain;

    /* Center of the AABB, which is used or view-frustum culling */
    glm::vec3 _aabbCenter;

    /* Actual center in the world space (y-coordinate read from the heightmap) */
    glm::vec3 _trueCenter;

    float _minY, _maxY;

    /* 2D translation to place the flat mesh to its actual center */
    glm::vec2 _translation;

    unsigned _currentLod;
    unsigned _blockId;
    unsigned _blockSize;

    /* Points defining the AABB */
    glm::vec3 _p1, _p2;

    /* The bitmap represents the bordering left, right, top, bottom blocks, where
     * each bit is either 1 if the bordering block has a lower LOD, otherwise 0 */
    unsigned _currentBorderBitmap;
};

#endif // GEOMIPMAPPINGBLOCK_H
