#ifndef GEOMIPMAPPING_H
#define GEOMIPMAPPING_H

#include "camera.h"
#include "terrain.h"

/**
 * @brief The GeoMipMappingPatch class
 */
class GeoMipMappingBlock {
public:
    float distance;
    unsigned int lod;
};

/**
 * @brief The GeoMipMapping class
 */
class GeoMipMapping : Terrain {
public:
    GeoMipMapping(std::string& heightmapFileName, std::string& textureFileName, unsigned int patchSize = 17);
    ~GeoMipMapping();
    void render(Camera camera);
    void loadBuffers();
    void unloadBuffers();
    void renderPatch();
    std::vector<GeoMipMappingBlock> blocks;
    unsigned int maxLod;
    unsigned int blockSize;
    unsigned int nBlocksX, nBlocksZ;
};

#endif // GEOMIPMAPPING_H
