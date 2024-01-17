#include "geomipmappingblock.h"

/**
 * @brief GeoMipMappingBlock::GeoMipMappingBlock
 * @param blockId
 * @param startIndex
 * @param center
 */
GeoMipMappingBlock::GeoMipMappingBlock(unsigned blockId, glm::vec3 trueCenter, glm::vec3 aabbCenter, unsigned blockSize)
{
    _blockId = blockId;
    _currentLod = 0;
    _currentBorderBitmap = 0;
    _aabbCenter = aabbCenter;
    _trueCenter = trueCenter;
    _blockSize = blockSize;
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
    Frustum frustum = camera.viewFrustum();

    return (checkPlane(frustum.leftFace) && checkPlane(frustum.rightFace) && checkPlane(frustum.topFace) && checkPlane(frustum.bottomFace) && checkPlane(frustum.nearFace) && checkPlane(frustum.farFace));
}

bool GeoMipMappingBlock::checkPlane(Plane& plane)
{
    float halfHeight = (_maxY - _minY) / 2.0f;
    float halfBlockSize = (float)_blockSize / 2.0f;
    float r = halfBlockSize * std::abs(plane.normal.x)
        + halfHeight * std::abs(plane.normal.y)
        + halfBlockSize * std::abs(plane.normal.z);

    return -r <= plane.getSignedDistanceToPlane(_aabbCenter);
}

void render(Camera& camera)
{
}
