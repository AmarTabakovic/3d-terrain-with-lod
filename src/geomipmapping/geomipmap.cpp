#include "geomipmap.h"

/**
 * @brief GeoMipMap::GeoMipMap
 * @param lod
 */
GeoMipMap::GeoMipMap(unsigned int lod)
{
    _lod = lod;
}

void GeoMipMap::lod(unsigned lod)
{
    _lod = lod;
}

unsigned GeoMipMap::lod()
{
    return _lod;
}
