#ifndef GEOMIPMAPPING_H
#define GEOMIPMAPPING_H

#include "camera.h"
#include "terrain.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

const unsigned int LEFT_BORDER_BITMASK = 0b1000;
const unsigned int RIGHT_BORDER_BITMASK = 0b0100;
const unsigned int TOP_BORDER_BITMASK = 0b0010;
const unsigned int BOTTOM_BORDER_BITMASK = 0b0001;

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

    /* Distance to the camera */
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

    /* A GeoMipMap can be accessed with the current LOD as the index */
    std::vector<GeoMipMap> geoMipMaps;

    unsigned int ebo;
    std::vector<unsigned int> indices;

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
    std::vector<unsigned int> borderStarts;
    std::vector<unsigned int> borderSizes;
    std::vector<unsigned int> centerStarts;
    std::vector<unsigned int> centerSizes;
};

/**
 * @brief The GeoMipMapping class
 */
class GeoMipMapping : public Terrain {
public:
    GeoMipMapping(std::string& heightmapFileName, std::string& textureFileName, unsigned int blockSize = 17);
    ~GeoMipMapping();
    void render(Camera camera);
    void loadBuffers();
    void unloadBuffers();

    void loadIndicesForBlock(unsigned int blockX, unsigned int blockY, unsigned int lod, unsigned int blockStart);
    // void renderPatch();
    void loadIndices();
    // void loadIndexBuffers();
    GeoMipMappingBlock& getBlock(unsigned int x, unsigned int z);
    unsigned int calculateBorderBitmap(unsigned int currentBlockId, unsigned int x, unsigned int y);
    void loadGeoMipMapsForBlock(GeoMipMappingBlock& block);
    unsigned int loadSingleBorderSide(GeoMipMappingBlock& block, unsigned int startIndex, unsigned int lod, unsigned int currentSide, bool leftSideHigher, bool rightSideHigher);
    unsigned int loadBorderAreaForConfiguration(GeoMipMappingBlock& block, unsigned int startIndex, unsigned int lod, unsigned int configuration);
    unsigned int loadBorderAreaForLod(GeoMipMappingBlock& block, unsigned int startIndex, unsigned int lod, unsigned int accumulatedCount);
    unsigned int loadCenterAreaForLod(GeoMipMappingBlock& block, unsigned int startIndex, unsigned int lod);
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
    // unsigned int trimmedWidth, trimmedHeight;
    std::vector<float> vertices;
};

#endif // GEOMIPMAPPING_H
