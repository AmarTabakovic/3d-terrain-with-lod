#ifndef GEOMIPMAPPINGBLOCK_H
#define GEOMIPMAPPINGBLOCK_H

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

class GeoMipMappingBlock {
public:
    GeoMipMappingBlock(unsigned blockId, glm::vec3 trueCenter, glm::vec3 p1, glm::vec3 p2, glm::vec2 translation);

    unsigned blockId;

    /* Actual center in the world space (y-coordinate read from the heightmap) */
    glm::vec3 _trueCenter;

    /* Points defining the AABB. _p1 contains minY and _p2 contains maxY */
    glm::vec3 p1, p2;

    /* 2D translation to place the flat mesh to its actual center */
    glm::vec2 translation;

    unsigned currentLod;

    /* The bitmap represents the bordering left, right, top, bottom blocks, where
     * each bit is either 1 if the bordering block has a lower LOD, otherwise 0 */
    unsigned currentBorderBitmap;
};

#endif // GEOMIPMAPPINGBLOCK_H
