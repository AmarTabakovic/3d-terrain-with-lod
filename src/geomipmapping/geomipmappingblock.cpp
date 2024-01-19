#include "geomipmappingblock.h"

GeoMipMappingBlock::GeoMipMappingBlock(unsigned blockId, glm::vec3 trueCenter, glm::vec3 p1, glm::vec3 p2, glm::vec2 translation)
{
    _blockId = blockId;
    _currentLod = 0;
    _currentBorderBitmap = 0;
    _trueCenter = trueCenter;
    _translation = translation;
    _p1 = p1;
    _p2 = p2;
}
