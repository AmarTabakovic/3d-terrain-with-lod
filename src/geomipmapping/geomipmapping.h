#ifndef GEOMIPMAPPING_H
#define GEOMIPMAPPING_H

#include "../camera.h"
#include "../terrain.h"
#include "geomipmappingblock.h"
#include "geomipmappingquadtree.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

/**
 * @brief Encapsulates the GeoMipMapping terrain rendering algorithm.
 *
 * The GeoMipMapping algorithm splits up the terrain into blocks of size
 * _blockSize, which must be of the form 2^n + 1. These blocks contain
 * only metadata, such as current LOD level, min. and max. Y-values,
 * its center in world space, etc.
 *
 * The terrain has a single vertex buffer of side length _blockSize
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
    /* Bitmasks for the 2^4 = 16 possible border configurations.
     * The bits are organized in left, right, top and bottom.
     * Each bit is set if the corresponding side has a lower LOD.
     */
    static const unsigned LEFT_BORDER_BITMASK = 0b1000;
    static const unsigned RIGHT_BORDER_BITMASK = 0b0100;
    static const unsigned TOP_BORDER_BITMASK = 0b0010;
    static const unsigned BOTTOM_BORDER_BITMASK = 0b0001;

    static const unsigned DEFAULT_BLOCK_SIZE = 65;
    static const unsigned DEFAULT_MIN_LOD = 0;
    static const unsigned DEFAULT_MAX_LOD = 10;

public:
    GeoMipMapping(Heightmap heightmap, float xzScale = 1.0f, float yScale = 1.0f, unsigned blockSize = DEFAULT_BLOCK_SIZE, unsigned minLod = DEFAULT_MIN_LOD, unsigned maxLod = DEFAULT_MAX_LOD);
    ~GeoMipMapping();

    /* Overriden virtual methods */
    void render(Camera camera);
    void loadBuffers();
    void unloadBuffers();

    /* Getters */
    unsigned nBlocksX();
    unsigned nBlocksZ();

    /* Setters */
    void baseDistance(float baseDistance);
    void doubleDistanceEachLevel(bool doubleDistanceEachLevel);

    bool _frustumCullingActive = true, _lodActive = true;
    bool _freezeCamera = false;

private:
    GeoMipMappingBlock& getBlock(unsigned x, unsigned z);
    unsigned calculateBorderBitmap(unsigned currentBlockId, unsigned x, unsigned y);
    unsigned determineLodDistance(float distance, float baseDist, bool doubleEachLevel = true);
    unsigned determineLodPaper(float distance);

    void loadIndices();
    void loadVertices();

    /* Note that the below could probably be refactored into fewer methods,
     * which I didn't manage to do yet due to time constraints. */
    void loadGeoMipMaps();
    unsigned loadBorderAreaForConfiguration(unsigned lod, unsigned configuration);
    unsigned loadBorderAreaForLod(unsigned lod, unsigned accumulatedCount);
    unsigned loadCenterAreaForLod(unsigned lod);

    unsigned loadTopLeftCorner(unsigned step, unsigned configuration);
    unsigned loadTopRightCorner(unsigned step, unsigned configuration);
    unsigned loadBottomRightCorner(unsigned step, unsigned configuration);
    unsigned loadBottomLeftCorner(unsigned step, unsigned configuration);

    unsigned loadTopBorder(unsigned configuration, unsigned step);
    unsigned loadRightBorder(unsigned configuration, unsigned step);
    unsigned loadBottomBorder(unsigned configuration, unsigned step);
    unsigned loadLeftBorder(unsigned configuration, unsigned step);

    /* LOD 0 and LOD 1 blocks are special since they do not have a center area,
     * but only a border area, which must be loaded specially. */
    unsigned loadLod0Block();
    unsigned loadLod1Block(unsigned configuration);

    GeoMipMappingQuadTree* root;

    std::vector<float> _vertices;
    std::vector<unsigned> indices;

    std::vector<GeoMipMappingBlock> _blocks;

    std::vector<unsigned> borderSizes;
    std::vector<unsigned> borderStarts;
    std::vector<unsigned> centerSizes;
    std::vector<unsigned> centerStarts;

    void pushIndex(unsigned x, unsigned y);

    Camera _lastCamera;

    unsigned _vao, _vbo, _ebo;

    unsigned _maxPossibleLod; /* Maximum possible number of LODs, calculated from block size */
    unsigned _minLod, _maxLod; /* Min. anx max. LOD level, defined by user */
    bool _doubleDistanceEachLevel;

//    bool _frustumCullingActive = true, _lodActive = true;
//    bool _freezeFrustumCulling = false, _freezeLod = false;

    unsigned _blockSize;

    float _baseDistance;

    /* The number of blocks on the x and z axis */
    unsigned _nBlocksX, _nBlocksZ;
};

#endif // GEOMIPMAPPING_H
