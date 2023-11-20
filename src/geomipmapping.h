#ifndef GEOMIPMAPPING_H
#define GEOMIPMAPPING_H

#include "camera.h"
#include "terrain.h"

/**
 * @brief The GeoMipMap class
 */
class GeoMipMap {
public:
    GeoMipMap(unsigned int lod);
    unsigned int lod;

    /* Each index buffer is for a border bitmap */
    std::vector<unsigned int> bufferSizes;

    /* TODO: Store index buffers contiguously in GeoMipMappingBlock instead*/
    std::vector<unsigned int> indexBuffers;
};

/**
 * @brief The GeoMipMappingPatch class
 */
class GeoMipMappingBlock {
public:
    GeoMipMappingBlock(unsigned int blockId, unsigned int startIndex);
    float distance;
    unsigned int currentLod;
    unsigned int blockId;

    /* Top-left corner of the block */
    unsigned int startIndex;

    /*
     * The bitmap represents the bordering left, right, top, bottom blocks, where
     * each bit is either 1 if the bordering block has a lower LOD, otherwise 0
     */
    unsigned int currentBorderBitmap;

    /* Access a mipmap with current LOD as index */
    std::vector<GeoMipMap> geoMipMaps;
};

/**
 * @brief The GeoMipMapping class
 */
class GeoMipMapping : public Terrain {
public:
    GeoMipMapping(std::string& heightmapFileName, std::string& textureFileName, unsigned int blockSize = 17);
    ~GeoMipMapping();
    void render(Camera camera);
    void loadIndicesForBlock(unsigned int blockX, unsigned int blockY, unsigned int lod, unsigned int blockStart);
    void loadBuffers();
    void unloadBuffers();
    void renderPatch();
    void loadIndices();
    void loadIndexBuffers();
    void loadGeoMipMapsForBlock(GeoMipMappingBlock& block);
    void loadBorderConfigurationsForGeoMipMap(GeoMipMap& geoMipMap, unsigned int startIndex);
    std::vector<unsigned int> createIndicesForConfig(unsigned int configBitmap, unsigned int lod, unsigned int startIndex);
    unsigned int determineLod(float distance);

    std::vector<GeoMipMappingBlock> blocks;
    unsigned int maxLod;
    unsigned int blockSize;

    /* The number of blocks on the x and z axis */
    unsigned int nBlocksX, nBlocksZ;

    /*
     * The trimmed width and height, such that the width and height are
     * a multiple of the block size
     */
    unsigned int trimmedWidth, trimmedHeight;
    std::vector<float> vertices;
};

#endif // GEOMIPMAPPING_H
