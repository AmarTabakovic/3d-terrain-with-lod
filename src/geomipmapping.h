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
    unsigned int count;

    /*
     * The bitmap represents the bordering left, right, top, bottom blocks, where
     * each bit is either 1 if the bordering block has a lower LOD, otherwise 0
     */
    unsigned int currentBorderBitmap;

    /* Access a mipmap with current LOD as index */
    std::vector<GeoMipMap> geoMipMaps;

    unsigned int ebo;
    std::vector<unsigned int> indexBuffer;

    /*
     * Each set of indices for a certain LOD level and border configuration,
     * e.g. (LOD 0, 0000), is stored on the same index buffer per block.
     *
     * To render different a block with a specific LOD and border configuration,
     * the start indices and sizes need to be known, which are stored in
     * subBufferSizes and subBufferStarts.
     */
    std::vector<unsigned int> subBufferSizes;
    std::vector<unsigned int> subBufferStarts;

    std::vector<unsigned int> borderSizes;
    std::vector<unsigned int> borderStarts;

    std::vector<unsigned int> centerSizes;
    std::vector<unsigned int> centerStarts;
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

    // NEW
    // unsigned int loadSingleBorderFan(unsigned int startIndex, unsigned int omissionConfig);
    unsigned int loadSingleBorderSide(GeoMipMappingBlock& block, unsigned int startIndex, unsigned int lod, unsigned int currentSide, bool leftSideHigher, bool rightSideHigher);
    unsigned int loadBorderAreaForConfiguration(GeoMipMappingBlock& block, unsigned int startIndex, unsigned int lod, unsigned int configuration);
    unsigned int loadBorderAreaForLod(GeoMipMappingBlock& block, unsigned int startIndex, unsigned int lod);
    unsigned int loadCenterAreaForLod(GeoMipMappingBlock& block, unsigned int startIndex, unsigned int lod);

    // OLD
    unsigned int loadBorderConfigurationsForGeoMipMap(GeoMipMappingBlock& block, unsigned int lod, unsigned int startIndex, unsigned int accumulatedCounts);
    unsigned int createIndicesForConfig(GeoMipMappingBlock& block, unsigned int configBitmap, unsigned int lod, unsigned int startIndex);

    unsigned int determineLod(float distance);

    unsigned int terrainVAO, terrainVBO;

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
