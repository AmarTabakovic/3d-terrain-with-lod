#ifndef GEOMIPMAPPINGBLOCK_H
#define GEOMIPMAPPINGBLOCK_H

#include "../camera.h"
#include "geomipmap.h"
// #include "geomipmapping.h"

#include <glm/vec3.hpp>
#include <vector>

class GeoMipMapping;

/**
 * @brief The GeoMipMappingPatch class
 *
 * TODO: Add method for pushing quads using two indices and step.
 */
class GeoMipMappingBlock {
public:
    GeoMipMappingBlock(unsigned blockId, unsigned startIndex, glm::vec3 center, unsigned blockSize);

    unsigned getRelativeIndex(unsigned width, unsigned x, unsigned y);
    void pushRelativeIndex(unsigned width, unsigned x, unsigned y); /* TODO: Store width as a field? */

    /* TODO: Define this method in camera class instead?
     *       And create class for AABB's? */
    bool insideViewFrustum(Camera& camera);
    bool checkPlane(Plane& plane);

    /* A GeoMipMap can be accessed with the current LOD as the index */
    std::vector<GeoMipMap> geoMipMaps;
    std::vector<unsigned> indices;

    GeoMipMapping* terrain;

    /*
     * The EBO of a block contains all indices of a block, for each LOD level
     * and border configuration. When rendering a block, the subset of the
     * indices for a certain LOD level and border configuration must be
     * selected, e.g. (LOD 0, 0000).
     *
     * The start indices and sizes of these subsets are stored
     * in the below fields borderStarts, borderSizes, centerStarts and
     * centerSizes.
     */
    std::vector<unsigned> borderStarts;
    std::vector<unsigned> borderSizes;
    std::vector<unsigned> centerStarts;
    std::vector<unsigned> centerSizes;

    /* World-space center for the distance calculation and AABB, which is used
     * for the frustum culling */
    glm::vec3 _center;

    /* Squared to save a square root call */
    float _squaredDistanceToCamera;

    unsigned _blockSize;
    unsigned _currentLod;
    unsigned _blockId;
    unsigned _ebo;
    unsigned _startIndex; /* Top-left corner of the block */

    /* The bitmap represents the bordering left, right, top, bottom blocks, where
     * each bit is either 1 if the bordering block has a lower LOD, otherwise 0 */
    unsigned _currentBorderBitmap;
};

#endif // GEOMIPMAPPINGBLOCK_H
