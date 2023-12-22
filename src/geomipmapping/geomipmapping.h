#ifndef GEOMIPMAPPING_H
#define GEOMIPMAPPING_H

#include "../camera.h"
#include "../terrain.h"
#include "geomipmappingblock.h"
#include "geomipmappingquadtree.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

/* Bitmasks for the 2^4 = 16 possible border configurations.
 * The bits are organized in left, right, top and bottom.
 * Each bit is set if the corresponding side has a lower LOD.
 */
const unsigned LEFT_BORDER_BITMASK = 0b1000;
const unsigned RIGHT_BORDER_BITMASK = 0b0100;
const unsigned TOP_BORDER_BITMASK = 0b0010;
const unsigned BOTTOM_BORDER_BITMASK = 0b0001;

const unsigned DEFAULT_BLOCK_SIZE = 65;

/**
 * @brief Encapsulates the GeoMipMapping terrain rendering algorithm.
 *
 * The GeoMipMapping algorithm splits up the terrain into blocks of size
 * _blockSize, which must be of the form 2^n + 1. The indices of a block
 * are cached in multiple LOD resolutions, which are called GeoMipMaps.
 * Each GeoMipMap is split up into the center and border area, and
 * each possible border configuration (represented by a bitmask of the
 * form left-right-top-bottom) is stored as well in the index buffer.
 *
 * As a general rule of thumb, the smaller the block size is, the more CPU
 * computations have to be performed per frame. So, for a small terrain,
 * small block sizes are appropriate, whereas for larger terrains, larger
 * block sizes should be considered.
 *
 * TODO: Support user-given maximum number of LODs
 */
class GeoMipMapping : public Terrain {
public:
    GeoMipMapping(Heightmap heightmap, float xzScale = 1.0f, float yScale = 1.0f, unsigned blockSize = DEFAULT_BLOCK_SIZE /*, unsigned numberOfLods */);
    ~GeoMipMapping();
    void render(Camera camera);
    void loadBuffers();
    void unloadBuffers();

private:
    GeoMipMappingBlock& getBlock(unsigned x, unsigned z);
    unsigned calculateBorderBitmap(unsigned currentBlockId, unsigned x, unsigned y);
    unsigned determineLodDistance(float distance, float baseDist, bool doubleEachLevel = true);
    unsigned determineLodPaper(float distance);

    void loadIndices();
    void loadNormals();
    void laodNormalsAsTexture();
    void calculateNormal(unsigned j, unsigned i, bool isBottomRight);

    /* ======================= Index loading methods ======================= */
    void loadIndicesForBlock(unsigned blockX, unsigned blockY, unsigned lod, unsigned blockStart);
    void loadGeoMipMapsForBlock(GeoMipMappingBlock& block);
    unsigned loadBorderAreaForConfiguration(GeoMipMappingBlock& block, unsigned startIndex, unsigned lod, unsigned configuration);
    unsigned loadBorderAreaForLod(GeoMipMappingBlock& block, unsigned startIndex, unsigned lod, unsigned accumulatedCount);
    unsigned loadCenterAreaForLod(GeoMipMappingBlock& block, unsigned startIndex, unsigned lod);

    /* TODO:
     * - Move these 4 methods to GeoMipMappingBlock
     * - Refactor these 4 into one method, reuse the same parts */
    unsigned loadTopLeftCorner(GeoMipMappingBlock& block, unsigned step, unsigned configuration);
    unsigned loadTopRightCorner(GeoMipMappingBlock& block, unsigned step, unsigned configuration);
    unsigned loadBottomRightCorner(GeoMipMappingBlock& block, unsigned step, unsigned configuration);
    unsigned loadBottomLeftCorner(GeoMipMappingBlock& block, unsigned step, unsigned configuration);
    /* TODO:
     * - Move these methods to GeoMipMappingBlock
     * - Refactor these 4 into one method, reuse the same parts */
    unsigned loadTopBorder(GeoMipMappingBlock& block, unsigned configuration, unsigned step);
    unsigned loadRightBorder(GeoMipMappingBlock& block, unsigned configuration, unsigned step);
    unsigned loadBottomBorder(GeoMipMappingBlock& block, unsigned configuration, unsigned step);
    unsigned loadLeftBorder(GeoMipMappingBlock& block, unsigned configuration, unsigned step);

    /* LOD 0 and LOD 1 blocks are special in the sense that they do not
     * have a center area, but only a border area, which must be
     * loaded specially. */
    unsigned loadLod0Block(GeoMipMappingBlock& block);
    unsigned loadLod1Block(GeoMipMappingBlock& block, unsigned configuration);

    GeoMipMappingQuadTree root;

    std::vector<float> _vertices;
    std::vector<glm::vec3> _normals;
    std::vector<GeoMipMappingBlock> _blocks;

    Camera _lastCamera;

    bool _viewFrustumCullingOn = true;

    unsigned _vao, _vbo;
    unsigned _maxLod; /* Maximum possible number of LODs, calculated from block size */
    unsigned _numberOfLods; /* User defined number of LODs */
    unsigned _blockSize;
    float _baseDistance = 1250; // 750;

    /* The number of blocks on the x and z axis */
    unsigned _nBlocksX, _nBlocksZ;

    unsigned _nIndicesPerBlock = 0;

    /* True if the normal map for shading should be generated at terrain
     * load time */
    bool _generateNormalMap;
};

#endif // GEOMIPMAPPING_H
