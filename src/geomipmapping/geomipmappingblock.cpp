#include "geomipmappingblock.h"

/**
 * @brief GeoMipMappingBlock::GeoMipMappingBlock
 * @param blockId
 * @param startIndex
 * @param center
 */
GeoMipMappingBlock::GeoMipMappingBlock(unsigned blockId, unsigned startIndex, glm::vec3 center, unsigned blockSize)
{
    _squaredDistanceToCamera = 0;
    _blockId = blockId;
    _startIndex = startIndex;
    _currentLod = 0;
    _currentBorderBitmap = 0;
    geoMipMaps = {};
    _center = center;
    _blockSize = blockSize;
}

/**
 * @brief GeoMipMappingBlock::pushRelativeIndex
 * @param width
 * @param x
 * @param y
 */
void GeoMipMappingBlock::pushRelativeIndex(unsigned width, unsigned x, unsigned y)
{
    indices.push_back(getRelativeIndex(width, x, y));
}

/**
 * @brief GeoMipMappingBlock::getRelativeIndex
 * @param width
 * @param x
 * @param y
 * @return
 */
unsigned GeoMipMappingBlock::getRelativeIndex(unsigned width, unsigned x, unsigned y)
{
    return y * width + _startIndex + x;
}

/**
 * @brief GeoMipMappingBlock::insideViewFrustum
 *
 * TODO: Define in Camera class instead
 *
 * @param camera
 * @return
 */
bool GeoMipMappingBlock::insideViewFrustum(Camera& camera)
{
    Frustum frustum = camera.viewFrustum;

    return (checkPlane(frustum.leftFace) && checkPlane(frustum.rightFace) && checkPlane(frustum.topFace) && checkPlane(frustum.bottomFace) && checkPlane(frustum.nearFace) && checkPlane(frustum.farFace));
}

bool GeoMipMappingBlock::checkPlane(Plane& plane)
{
    float halfBlockSize = (float)_blockSize / 2;
    float r = halfBlockSize * std::abs(plane.normal.x)
        + halfBlockSize * std::abs(plane.normal.y)
        + halfBlockSize * std::abs(plane.normal.z);

    return -r <= plane.getSignedDistanceToPlane(_center);
}
