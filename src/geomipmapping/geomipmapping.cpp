#include "geomipmapping.h"
#include "../atlodutil.h"

#include <algorithm>
#include <cmath>

#include <chrono>

void GeoMipMapping::loadBuffers()
{
    loadVertices();
    loadIndices();
}

void GeoMipMapping::loadVertices()
{
    for (int i = 0; i < _blockSize; i++) {
        for (int j = 0; j < _blockSize; j++) {
            /* Load vertices around center point */
            float x = (-(float)_blockSize / 2.0f + (float)_blockSize * j / (float)_blockSize);
            float z = (-(float)_blockSize / 2.0f + (float)_blockSize * i / (float)_blockSize);

            _vertices.push_back(x); /* Position x */
            _vertices.push_back(z); /* Position z */
        }
    }

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(float), &_vertices[0], GL_STATIC_DRAW);

    /* Position attribute */
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void GeoMipMapping::loadIndices()
{
    unsigned totalCount = 0;

    if (_minLod == 0) {
        /* ========================== Load LOD 0 block ==========================*/
        unsigned lod0Count = loadLod0Block();
        totalCount += lod0Count;

        /* For LOD 0, the (single) border block is always the same */
        for (int i = 0; i < 16; i++) {
            borderStarts.push_back(totalCount - lod0Count);
            borderSizes.push_back(lod0Count);
        }

        /* LOD 0 border block does not have a center */
        centerStarts.push_back(0);
        centerSizes.push_back(0);
    }

    if (_minLod == 0 || _minLod == 1) {

        /* ========================== Load LOD 1 block ==========================*/
        for (unsigned i = 0; i < 16; i++) {
            unsigned lod1Count = loadLod1Block(i);
            totalCount += lod1Count;
            borderStarts.push_back(totalCount - lod1Count);
            borderSizes.push_back(lod1Count);
        }

        /* LOD 1 border block does not have a center */
        centerStarts.push_back(0);
        centerSizes.push_back(0);
        // block.geoMipMaps.push_back(GeoMipMap(1));
    }

    /* ============================= Load rest ==============================*/
    for (unsigned i = std::max(_minLod, 2u) /*2*/; i <= _maxLod; i++) {
        /* Load border subblocks */
        unsigned borderCount = loadBorderAreaForLod(i, totalCount);

        totalCount += borderCount;

        /* Load center subblocks */
        unsigned centerCount = loadCenterAreaForLod(i);
        totalCount += centerCount;
        centerStarts.push_back(totalCount - centerCount);
    }

    std::cout << indices.size() << std::endl;

    glGenBuffers(1, &_ebo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), &indices[0], GL_STATIC_DRAW);
}

/**
 * @brief GeoMipMapping::render
 * @param camera
 */
void GeoMipMapping::render(Camera camera)
{
    shader().use();
    shader().setFloat("yScale", _yScale);

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RESTART_INDEX);

    if (!_freezeCamera)
        _lastCamera = camera;

    /* ================================ First pass ===============================
     * - For each block:
     *   - Check and update the distance to camera
     *   - Update the LOD level
     *   - Update the neighborhood border bitmap */
    for (unsigned i = 0; i < _nBlocksZ; i++) {
        for (unsigned j = 0; j < _nBlocksX; j++) {
            GeoMipMappingBlock& block = getBlock(j, i);

            glm::vec3 temp = block._trueCenter - _lastCamera.position();
            float squaredDistance = glm::dot(temp, temp);

            //block._squaredDistanceToCamera = squaredDistance;
            //unsigned currentLod = block._currentLod;
            if (!_lodActive)
                block._currentLod = _maxLod;
            else if (!_freezeCamera)
                block._currentLod = determineLodDistance(squaredDistance, _baseDistance, _doubleDistanceEachLevel);

            block._currentBorderBitmap = calculateBorderBitmap(block._blockId, j, i);
        }
    }

    glBindVertexArray(_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);

    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, heightmapTextureId);
    shader().setInt("heightmapTexture", 1);

    // TODO
    // root.render(camera);

    /* ============================== Second pass =============================
     * - For each block:
     *   - Check frustum culling
     *   - Render center and border subblocks
     */
    for (unsigned i = 0; i < _nBlocksZ; i++) {
        for (unsigned j = 0; j < _nBlocksX; j++) {
            //glActiveTexture(GL_TEXTURE1);
            //glBindTexture(GL_TEXTURE_2D, heightmapTextureId);
            //shader().setInt("heightmapTexture", 1);
            GeoMipMappingBlock& currblock = getBlock(j, i);

            if (_hasTexture) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, _textureId);
                shader().setFloat("doTexture", 1.0f);
            } else
                shader().setFloat("doTexture", 0.0f);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, _heightmap.heightmapTextureId());

            /* TODO:
             * - Collect visible blocks in first pass
             * - Quadtrees */
            if (_frustumCullingActive && !currblock.insideViewFrustum(_lastCamera))
               continue;

            unsigned currentIndex = currblock._currentLod;

            float r = 0.3f, g = 0.3f, b = 0.3f;

            if (currblock._currentLod % 3 == 0)
                r = 0.7f;
            else if (currblock._currentLod % 3 == 1)
                g = 0.7f;
            else
                b = 0.7f;

            shader().setVec4("inColor", glm::vec4(r, g, b, 1.0f));

            shader().setFloat("textureWidth", _heightmap.width());
            shader().setFloat("textureHeight", _heightmap.height());
            shader().setVec2("offset", currblock.translation);

            shader().setFloat("maxT", _heightmap.max);
            shader().setFloat("minT", _heightmap.min);

            currentIndex -= _minLod;

            /* First render the center subblocks (only for LOD >= 2, since
             * LOD 0 and 1 do not have a center block) */
            if (currblock._currentLod >= 2) {
                glDrawElements(GL_TRIANGLE_STRIP,
                    centerSizes[currentIndex],
                    GL_UNSIGNED_INT,
                    (void*)(centerStarts[currentIndex] * sizeof(unsigned)));
            }

            /* Then render the border subblocks
             * Note: This couild probably be optimized with
             * glMultiDrawElements() */
            currentIndex = currentIndex * 16 + currblock._currentBorderBitmap;
            glDrawElements(GL_TRIANGLE_STRIP,
                borderSizes[currentIndex],
                GL_UNSIGNED_INT,
                (void*)(borderStarts[currentIndex] * sizeof(unsigned)));
        }
    }

    AtlodUtil::checkGlError("GeoMipMapping render failed");
}

/**
 * @brief GeoMipMapping::GeoMipMapping
 *
 * If numLods >= maxLod, then numLods gets ignored during index loading.
 *
 * @param heightmap
 * @param xzScale
 * @param yScale
 * @param blockSize
 * @param numLods
 */
GeoMipMapping::GeoMipMapping(Heightmap heightmap, float xzScale, float yScale, unsigned blockSize, unsigned minLod, unsigned maxLod)
{
    std::cout << "Initialize GeoMipMapping" << std::endl;

    _xzScale = xzScale;
    _yScale = yScale;
    _blockSize = blockSize;
    _heightmap = heightmap;
    _shader = Shader("../3d-terrain-with-lod/src/glsl/geomipmapping.vert", "../3d-terrain-with-lod/src/glsl/geomipmapping.frag");

    /* Always floor so that we do not "overshoot" when multiplying the number
     * of blocks with the block size */
    _nBlocksX = std::floor((heightmap.width() - 1) / (blockSize - 1));
    _nBlocksZ = std::floor((heightmap.height() - 1) / (blockSize - 1));

    /* Plus one is important */
    _width = _nBlocksX * (blockSize - 1) + 1;
    _height = _nBlocksZ * (blockSize - 1) + 1;

    /* Check whether block size is of the form 2^n + 1*/
    if (((blockSize - 1) & (blockSize - 2)) != 0) {
        std::cerr << "Block size must be of the form 2^n + 1";
        std::exit(1);
    }

    /* Calculate maximum LOD level */
    _maxPossibleLod = std::log2(blockSize - 1);

    /* Calculate user defined LOD level bounds */
    _maxLod = std::min(maxLod, _maxPossibleLod);
    _minLod = std::max(0u, minLod);

    if (_minLod > maxLod) {
        std::cerr << "Error: Max LOD cannot be less than Min LOD" << std::endl;
        std::exit(1);
    }

    //loadHeightmapTexture();
    //_heightmap.generateGlTexture();
    shader().use();
    shader().setInt("texture1", 0);
    shader().setInt("heightmapTexture", 1);

    glm::vec2 terrainCenter(_width / 2.0f, _height / 2.0f);
    /* Generate blocks */
    for (unsigned i = 0; i < _nBlocksZ; i++) {
        for (unsigned j = 0; j < _nBlocksX; j++) {
            /* Determine min and max y-coordinates per block for the AABB */
            float minY = 9999999.0f, maxY = -9999999.0f;

            for (unsigned k = 0; k < blockSize; k++) {
                for (unsigned l = 0; l < blockSize; l++) {
                    float x = (j * (blockSize - 1)) + l;
                    float z = (i * (blockSize - 1)) + k;

                    float y = heightmap.at(x, z);
                    minY = std::min(y, minY);
                    maxY = std::max(y, maxY);
                }
            }

            unsigned currentBlockId = i * _nBlocksX + j;

            float centerX = (j * (_blockSize - 1) + 0.5 * (_blockSize - 1));
            float centerZ = (i * (_blockSize - 1) + 0.5 * (_blockSize - 1));

            float trueY = heightmap.at(centerX, centerZ) * yScale;
            float aabbY = minY * yScale + ((maxY * yScale - minY * yScale) / (2.0f * yScale));

            glm::vec3 aabbCenter(((-(float)_width * _xzScale) / 2.0f) + centerX * _xzScale, aabbY, ((-(float)_height * _xzScale) / 2.0f) + centerZ * _xzScale);
            glm::vec3 blockCenter(aabbCenter.x, trueY, aabbCenter.z);

            GeoMipMappingBlock block = GeoMipMappingBlock(currentBlockId, blockCenter, aabbCenter, _blockSize);
            block._minY = minY;
            block._maxY = maxY;
            block.translation = glm::vec2(centerX, centerZ) - terrainCenter;

            //std::cout << "===== BLOCK " << currentBlock << " =====\n";
            //std::cout << "x, y: " << j << ", " << i << std::endl;
            //std::cout << "blockCenter: " << blockCenter.x << ", " << blockCenter.y << ", " << blockCenter.z << std::endl;

            _blocks.push_back(block);
        }
    }
    std::cout << "Finished blocks\n";
    // TODO
    // root.build();
}

unsigned GeoMipMapping::nBlocksX()
{
    return _nBlocksX;
}

unsigned GeoMipMapping::nBlocksZ()
{
    return _nBlocksZ;
}

/**
 * @brief GeoMipMapping::~GeoMipMapping
 */
GeoMipMapping::~GeoMipMapping()
{
    std::cout << "GeoMipMapping terrain destroyed" << std::endl;
}

/**
 * @brief GeoMipMapping::calculateBorderBitmap
 * @param currentBlockId
 * @param x
 * @param y
 * @return
 */
unsigned GeoMipMapping::calculateBorderBitmap(unsigned currentBlockId, unsigned x, unsigned z)
{
    unsigned currentLod = _blocks[currentBlockId]._currentLod;

    unsigned maxX = std::max((int)x - 1, 0);
    unsigned minX = std::min((int)x + 1, (int)_nBlocksX - 1);
    unsigned maxZ = std::max((int)z - 1, 0);
    unsigned minZ = std::min((int)z + 1, (int)_nBlocksZ - 1);

    GeoMipMappingBlock& leftBlock = getBlock(maxX, z);
    GeoMipMappingBlock& rightBlock = getBlock(minX, z);
    GeoMipMappingBlock& topBlock = getBlock(x, maxZ);
    GeoMipMappingBlock& bottomBlock = getBlock(x, minZ);

    unsigned leftLower = currentLod > leftBlock._currentLod ? 1 : 0;
    unsigned rightLower = currentLod > rightBlock._currentLod ? 1 : 0;
    unsigned topLower = currentLod > topBlock._currentLod ? 1 : 0;
    unsigned bottomLower = currentLod > bottomBlock._currentLod ? 1 : 0;

    unsigned bitmap = (leftLower << 3) | (rightLower << 2) | (topLower << 1) | bottomLower;

    return bitmap;
}

void GeoMipMapping::baseDistance(float baseDistance)
{
    _baseDistance = baseDistance;
}

void GeoMipMapping::doubleDistanceEachLevel(bool doubleDistanceEachLevel)
{
    _doubleDistanceEachLevel = doubleDistanceEachLevel;
}

/**
 * @brief GeoMipMapping::getBlock
 *
 * TODO: Error handling
 *
 * @param x
 * @param y
 * @return
 */
GeoMipMappingBlock& GeoMipMapping::getBlock(unsigned x, unsigned z)
{
    return _blocks[z * _nBlocksX + x];
}


/**
 * @brief Determines the LOD level using the distance from the camera and a
 *        given base distance.
 *
 * @param distance - the distance from the center of the block to the camera
 * @param baseDist - the distance where a single LOD level is valid
 * @param doubleEachLevel - true if the distance of each subsequent LOD level should get doubled
 * @return the LOD level
 */
unsigned GeoMipMapping::determineLodDistance(float distance, float baseDist, bool doubleEachLevel)
{
    unsigned distancePower = 1;
    for (int i = 0; i < _maxLod - _minLod; i++) {
        if (distance < distancePower * distancePower * baseDist * baseDist)
            return _maxLod - i;

        if (doubleEachLevel)
            distancePower <<= 1;
        else
            distancePower++;
    }
    return _minLod; // 0
}

void GeoMipMapping::pushIndex(unsigned x, unsigned y)
{
    indices.push_back(y * _blockSize + x);
}

/**
 * @brief GeoMipMapping::loadCenterAreaForLod
 * @param block
 * @param startIndex
 * @param lod the current LOD level
 * @return the total number of indices loaded
 */
unsigned GeoMipMapping::loadCenterAreaForLod(unsigned lod)
{
    unsigned step = std::pow(2, _maxLod - lod);
    unsigned count = 0;

    for (unsigned i = step; i < _blockSize - step - 1; i += step) {
        for (unsigned j = step; j < _blockSize - step; j += step) {
            pushIndex(j, i);
            pushIndex(j, i + step);
            count += 2;
        }
        indices.push_back(RESTART_INDEX);
        count++;
    }

    centerSizes.push_back(count);

    return count;
}

/**
 * @brief GeoMipMapping::loadBorderAreaForLod
 * @param block
 * @param startIndex
 * @param lod
 * @return
 */
unsigned GeoMipMapping::loadBorderAreaForLod(unsigned lod, unsigned accumulatedCount)
{
    unsigned totalCount = 0;

    /* 2^4 = 16 possible combinations */
    for (int i = 0; i < 16; i++) {
        unsigned count = loadBorderAreaForConfiguration(lod, i);
        totalCount += count;
        accumulatedCount += count;
        borderStarts.push_back(accumulatedCount - count);
    }

    return totalCount;
}

/**
 * @brief GeoMipMapping::load2x2Block
 * @return
 */
unsigned GeoMipMapping::loadLod0Block()
{
    pushIndex(0, 0);
    pushIndex(0, _blockSize - 1);
    pushIndex(_blockSize - 1, 0);
    pushIndex(_blockSize - 1, _blockSize - 1);
    indices.push_back(RESTART_INDEX);

    return 5;
}

/**
 * @brief Loads a single LOD 1 (i.e. 3x3) block.
 * @return Number of indices generated
 */
unsigned GeoMipMapping::loadLod1Block(unsigned configuration)
{
    unsigned count = 0;
    unsigned step = std::pow(2, _maxLod - 1);

    /* ============== Block is surrounded by lower LOD blocks ============== */
    if (configuration == 0b1111) {
        pushIndex(0, 0);
        pushIndex(step, step);
        pushIndex(_blockSize - 1, 0);
        pushIndex(_blockSize - 1, _blockSize - 1);
        indices.push_back(RESTART_INDEX);

        pushIndex(0, 0);
        pushIndex(0, _blockSize - 1);
        pushIndex(step, step);
        pushIndex(_blockSize - 1, _blockSize - 1);
        indices.push_back(RESTART_INDEX);
        count += 10;
    }

    /* ======= Block is surrounded by exactly three lower LOD blocks ======= */
    else if (configuration == 0b1110 || configuration == 0b1101 || configuration == 0b1011 || configuration == 0b00111) {
        /* Left or bottom block has the same LOD */
        if (configuration == 0b1110 || configuration == 0b0111) {

            pushIndex(0, 0);
            pushIndex(step, step);
            pushIndex(_blockSize - 1, 0);
            pushIndex(_blockSize - 1, _blockSize - 1);
            indices.push_back(RESTART_INDEX);
            count += 5;

            /* Bottom block has the same LOD*/
            if (configuration == 0b1110) {

                pushIndex(_blockSize - 1, _blockSize - 1);
                pushIndex(step, step);
                pushIndex(step, _blockSize - 1);
                pushIndex(0, _blockSize - 1);
                indices.push_back(RESTART_INDEX);

                pushIndex(0, _blockSize - 1);
                pushIndex(step, step);
                pushIndex(0, 0);
                indices.push_back(RESTART_INDEX);

                count += 9;
            } else { /* Left block has the same LOD */
                pushIndex(0, _blockSize - 1);
                pushIndex(step, step);
                pushIndex(0, step);
                pushIndex(0, 0);
                indices.push_back(RESTART_INDEX);

                pushIndex(_blockSize - 1, _blockSize - 1);
                pushIndex(step, step);
                pushIndex(0, _blockSize - 1);
                indices.push_back(RESTART_INDEX);

                count += 9;
            }

        } else { /* Top or right block has the same LOD */

            pushIndex(0, 0);
            pushIndex(0, _blockSize - 1);
            pushIndex(step, step);
            pushIndex(_blockSize - 1, _blockSize - 1);
            indices.push_back(RESTART_INDEX);

            count += 5;

            /* Top block has the same LOD*/
            if (configuration == 0b1101) {

                pushIndex(0, 0);
                pushIndex(step, step);
                pushIndex(step, 0);
                pushIndex(_blockSize - 1, 0);
                indices.push_back(RESTART_INDEX);

                pushIndex(step, step);
                pushIndex(_blockSize - 1, _blockSize - 1);
                pushIndex(_blockSize - 1, 0);
                indices.push_back(RESTART_INDEX);

                count += 9;

            } else { /* Right block has the same LOD */

                pushIndex(_blockSize - 1, 0);
                pushIndex(step, step);
                pushIndex(_blockSize - 1, step);
                pushIndex(_blockSize - 1, _blockSize - 1);
                indices.push_back(RESTART_INDEX);

                pushIndex(0, 0);
                pushIndex(step, step);
                pushIndex(_blockSize - 1, 0);
                indices.push_back(RESTART_INDEX);

                count += 9;
            }
        }
    }

    /* ======== Block is surrounded by exaclty two lower LOD blocks ======== */
    else if (configuration == 0b0011 || configuration == 0b1100) {
        if (configuration == 0b0011) {

            pushIndex(_blockSize - 1, step);
            pushIndex(_blockSize - 1, 0);
            pushIndex(step, step);
            pushIndex(0, 0);
            pushIndex(0, _blockSize - 1);
            indices.push_back(RESTART_INDEX);

            pushIndex(0, step);
            pushIndex(0, _blockSize - 1);
            pushIndex(step, step);
            pushIndex(_blockSize - 1, _blockSize - 1);
            pushIndex(_blockSize - 1, step);
            indices.push_back(RESTART_INDEX);

            count += 12;
        } else {

            pushIndex(step, 0);
            pushIndex(0, 0);
            pushIndex(step, step);
            pushIndex(0, _blockSize - 1);
            pushIndex(step, _blockSize - 1);
            indices.push_back(RESTART_INDEX);

            pushIndex(step, _blockSize - 1);
            pushIndex(_blockSize - 1, _blockSize - 1);
            pushIndex(step, step);
            pushIndex(_blockSize - 1, 0);
            pushIndex(step, 0);
            indices.push_back(RESTART_INDEX);

            count += 12;
        }
    }

    /* ==== Determine which corner is a regular quad and the method of the ====
     *      opposing corner */
    else if (!(configuration & (LEFT_BORDER_BITMASK | TOP_BORDER_BITMASK))) {
        count += loadBottomRightCorner(step, configuration);

        pushIndex(0, 0);
        pushIndex(0, step);
        pushIndex(step, 0);
        pushIndex(step, step);
        indices.push_back(RESTART_INDEX);
        count += 5;
    } else if (!(configuration & (TOP_BORDER_BITMASK | RIGHT_BORDER_BITMASK))) {
        count += loadBottomLeftCorner(step, configuration);

        pushIndex(step, 0);
        pushIndex(step, step);
        pushIndex(_blockSize - 1, 0);
        pushIndex(_blockSize - 1, step);
        indices.push_back(RESTART_INDEX);
        count += 5;

    } else if (!(configuration & (RIGHT_BORDER_BITMASK | BOTTOM_BORDER_BITMASK))) {
        count += loadTopLeftCorner(step, configuration);

        pushIndex(step, step);
        pushIndex(step, _blockSize - 1);
        pushIndex(_blockSize - 1, step);
        pushIndex(_blockSize - 1, _blockSize - 1);
        indices.push_back(RESTART_INDEX);
        count += 5;
    } else if (!(configuration & (BOTTOM_BORDER_BITMASK | LEFT_BORDER_BITMASK))) {
        count += loadTopRightCorner(step, configuration);

        pushIndex(0, step);
        pushIndex(0, _blockSize - 1);
        pushIndex(step, step);
        pushIndex(step, _blockSize - 1);
        indices.push_back(RESTART_INDEX);
        count += 5;
    }

    return count;
}

/**
 * @brief GeoMipMapping::loadBorderAreaForConfiguration
 * @param block
 * @param startIndex
 * @param lod
 * @param configuration
 * @return
 */
unsigned GeoMipMapping::loadBorderAreaForConfiguration(unsigned lod, unsigned configuration)
{

    /* The idea is to traverse the border subblocks clockwise starting from
     * the top left corner.
     *
     * TODO:
     * - Refactor the below left, right, top, bottom cases into single methods,
     *   same for corners */

    unsigned step = std::pow(2, _maxLod - lod);
    unsigned count = 0;

    count += loadTopLeftCorner(step, configuration);
    count += loadTopBorder(step, configuration);
    count += loadTopRightCorner(step, configuration);
    count += loadRightBorder(step, configuration);
    count += loadBottomRightCorner(step, configuration);
    count += loadBottomBorder(step, configuration);
    count += loadBottomLeftCorner(step, configuration);
    count += loadLeftBorder(step, configuration);

    borderSizes.push_back(count);

    return count;
}

/**
 * @brief GeoMipMapping::loadTopLeftcorner
 * @param block
 * @param step
 * @param configuration
 * @return
 */
unsigned GeoMipMapping::loadTopLeftCorner(unsigned step, unsigned configuration)
{
    unsigned count = 0;
    if ((configuration & LEFT_BORDER_BITMASK) && (configuration & TOP_BORDER_BITMASK)) { /* bitmask is 1_1_ */
        /*
         * *- - -*- - -*
         * |\         /|
         * |  \     /  |
         * |    \ /    |
         * *     *- - -*
         * |    /|
         * |  /  |
         * |/    |
         * *- - -*
         *
         */

        pushIndex(2 * step, step);
        pushIndex(2 * step, 0);
        pushIndex(step, step);
        pushIndex(0, 0);
        pushIndex(0, 2 * step);
        indices.push_back(RESTART_INDEX);

        pushIndex(step, 2 * step);
        pushIndex(step, step);
        pushIndex(0, 2 * step);
        indices.push_back(RESTART_INDEX);

        count += 10;

    } else if (configuration & LEFT_BORDER_BITMASK) { /* bitmask is 1_0_*/

        /*
         * *- - -*- - -*
         * |\    |    /|
         * |  \  |  /  |
         * |    \|/    |
         * *     *- - -*
         * |    /|
         * |  /  |
         * |/    |
         * *- - -*
         *
         */

        pushIndex(step, 0);
        pushIndex(0, 0);
        pushIndex(step, step);
        pushIndex(0, 2 * step);
        pushIndex(step, 2 * step);
        indices.push_back(RESTART_INDEX);

        pushIndex(step, 0);
        pushIndex(step, step);
        pushIndex(2 * step, 0);
        pushIndex(2 * step, step);
        indices.push_back(RESTART_INDEX);

        count += 11;

    } else if (configuration & TOP_BORDER_BITMASK) { /* bitmask is 0_1_ */
        /*
         * *- - -*- - -*
         * |\         /|
         * |  \     /  |
         * |    \ /    |
         * *- - -*- - -*
         * |    /|
         * |  /  |
         * |/    |
         * *- - -*
         *
         */

        pushIndex(0, step);
        pushIndex(0, 2 * step);
        pushIndex(step, step);
        pushIndex(step, 2 * step);
        indices.push_back(RESTART_INDEX);

        pushIndex(2 * step, step);
        pushIndex(2 * step, 0);
        pushIndex(step, step);
        pushIndex(0, 0);
        pushIndex(0, step);
        indices.push_back(RESTART_INDEX);

        count += 11;

    } else { /* bitmask is 0_0_*/
        /*
         * *- - -*- - -*
         * |    /|    /|
         * |  /  |  /  |
         * |/    |/    |
         * *- - -*- - -*
         * |    /|
         * |  /  |
         * |/    |
         * *- - -*
         *
         */

        pushIndex(0, step);
        pushIndex(0, 2 * step);
        pushIndex(step, step);
        pushIndex(step, 2 * step);
        indices.push_back(RESTART_INDEX);

        pushIndex(0, 0);
        pushIndex(0, step);
        pushIndex(step, 0);
        pushIndex(step, step);
        pushIndex(2 * step, 0);
        pushIndex(2 * step, step);
        indices.push_back(RESTART_INDEX);

        count += 12;
    }

    return count;
}

/**
 * @brief GeoMipMapping::loadTopRightCorner
 * @param block
 * @param step
 * @param configuration
 * @return
 */
unsigned GeoMipMapping::loadTopRightCorner(unsigned step, unsigned configuration)
{
    unsigned count = 0;
    if ((configuration & RIGHT_BORDER_BITMASK) && (configuration & TOP_BORDER_BITMASK)) { /* bitmask is _11_ */
        /*
         * *- - -*- - -*
         * |\         /|
         * |  \     /  |
         * |    \ /    |
         * *- - -*     *
         *       |\    |
         *       |  \  |
         *       |    \|
         *       *- - -*
         *
         */

        pushIndex(_blockSize - 1 - step, 2 * step);
        pushIndex(_blockSize - 1, 2 * step);
        pushIndex(_blockSize - 1 - step, step);
        pushIndex(_blockSize - 1, 0);
        pushIndex(_blockSize - 1 - 2 * step, 0);
        indices.push_back(RESTART_INDEX);

        pushIndex(_blockSize - 1 - step, step);
        pushIndex(_blockSize - 1 - 2 * step, 0);
        pushIndex(_blockSize - 1 - 2 * step, step);
        indices.push_back(RESTART_INDEX);

        count += 10;

    } else if (configuration & RIGHT_BORDER_BITMASK) { /* bitmask is _10_*/

        /*
         * *- - -*- - -*
         * |    /|    /|
         * |  /  |  /  |
         * |/    |/    |
         * *- - -*     *
         *       |\    |
         *       |  \  |
         *       |    \|
         *       *- - -*
         */

        pushIndex(_blockSize - 1 - step, 2 * step);
        pushIndex(_blockSize - 1, 2 * step);
        pushIndex(_blockSize - 1 - step, step);
        pushIndex(_blockSize - 1, 0);
        pushIndex(_blockSize - 1 - step, 0);
        indices.push_back(RESTART_INDEX);

        pushIndex(_blockSize - 1 - 2 * step, 0);
        pushIndex(_blockSize - 1 - 2 * step, step);
        pushIndex(_blockSize - 1 - step, 0);
        pushIndex(_blockSize - 1 - step, step);
        indices.push_back(RESTART_INDEX);

        count += 11;

    } else if (configuration & TOP_BORDER_BITMASK) { /* bitmask is _01_ */
        /*
         * *- - -*- - -*
         * |\         /|
         * |  \     /  |
         * |    \ /    |
         * *- - -*- - -*
         *       |    /|
         *       |  /  |
         *       |/    |
         *       *- - -*
         *
         */

        pushIndex(_blockSize - 1 - step, step);
        pushIndex(_blockSize - 1 - step, 2 * step);
        pushIndex(_blockSize - 1, step);
        pushIndex(_blockSize - 1, 2 * step);
        indices.push_back(RESTART_INDEX);

        pushIndex(_blockSize - 1, step);
        pushIndex(_blockSize - 1, 0);
        pushIndex(_blockSize - 1 - step, step);
        pushIndex(_blockSize - 1 - 2 * step, 0);
        pushIndex(_blockSize - 1 - 2 * step, step);
        indices.push_back(RESTART_INDEX);

        count += 11;

    } else { /* bitmask is _00_*/
        /*
         * *- - -*- - -*
         * |    /|    /|
         * |  /  |  /  |
         * |/    |/    |
         * *- - -*- - -*
         *       |    /|
         *       |  /  |
         *       |/    |
         *       *- - -*
         *
         */

        pushIndex(_blockSize - 1 - 2 * step, 0);
        pushIndex(_blockSize - 1 - 2 * step, step);
        pushIndex(_blockSize - 1 - step, 0);
        pushIndex(_blockSize - 1 - step, step);
        pushIndex(_blockSize - 1, 0);
        pushIndex(_blockSize - 1, step);
        indices.push_back(RESTART_INDEX);

        pushIndex(_blockSize - 1 - step, step);
        pushIndex(_blockSize - 1 - step, 2 * step);
        pushIndex(_blockSize - 1, step);
        pushIndex(_blockSize - 1, 2 * step);
        indices.push_back(RESTART_INDEX);

        count += 12;
    }

    return count;
}

/**
 * @brief GeoMipMapping::loadBottomRightCorner
 * @param block
 * @param step
 * @param configuration
 * @return
 */
unsigned GeoMipMapping::loadBottomRightCorner(unsigned step, unsigned configuration)
{
    unsigned count = 0;

    if ((configuration & RIGHT_BORDER_BITMASK) && (configuration & BOTTOM_BORDER_BITMASK)) { /* bitmask is _1_1 */
        /*
         *       *- - -*
         *       |    /|
         *       |  /  |
         *       |/    |
         * *- - -*     *
         * |    / \    |
         * |  /     \  |
         * |/         \|
         * *- - -*- - -*
         *
         */

        pushIndex(_blockSize - 1 - 2 * step, _blockSize - 1 - step);
        pushIndex(_blockSize - 1 - 2 * step, _blockSize - 1);
        pushIndex(_blockSize - 1 - step, _blockSize - 1 - step);
        pushIndex(_blockSize - 1, _blockSize - 1);
        pushIndex(_blockSize - 1, _blockSize - 1 - 2 * step);
        indices.push_back(RESTART_INDEX);

        pushIndex(_blockSize - 1 - step, _blockSize - 1 - 2 * step);
        pushIndex(_blockSize - 1 - step, _blockSize - 1 - step);
        pushIndex(_blockSize - 1, _blockSize - 1 - 2 * step);
        indices.push_back(RESTART_INDEX);

        count += 10;

    } else if (configuration & RIGHT_BORDER_BITMASK) { /* bitmask is _1_0*/

        /*
         *       *- - -*
         *       |    /|
         *       |  /  |
         *       |/    |
         * *- - -*     *
         * |    /|\    |
         * |  /  |  \  |
         * |/    |    \|
         * *- - -*- - -*
         *
         */

        pushIndex(_blockSize - 1 - step, _blockSize - 1);
        pushIndex(_blockSize - 1, _blockSize - 1);
        pushIndex(_blockSize - 1 - step, _blockSize - 1 - step);
        pushIndex(_blockSize - 1, _blockSize - 1 - 2 * step);
        pushIndex(_blockSize - 1 - step, _blockSize - 1 - 2 * step);
        indices.push_back(RESTART_INDEX);

        pushIndex(_blockSize - 1 - 2 * step, _blockSize - 1 - step);
        pushIndex(_blockSize - 1 - 2 * step, _blockSize - 1);
        pushIndex(_blockSize - 1 - step, _blockSize - 1 - step);
        pushIndex(_blockSize - 1 - step, _blockSize - 1);
        indices.push_back(RESTART_INDEX);

        count += 11;

    } else if (configuration & BOTTOM_BORDER_BITMASK) { /* bitmask is _0_1 */
        /*
         *       *- - -*
         *       |    /|
         *       |  /  |
         *       |/    |
         * *- - -*- - -*
         * |    / \    |
         * |  /     \  |
         * |/         \|
         * *- - -*- - -*
         *
         */

        pushIndex(_blockSize - 1 - step, _blockSize - 1 - 2 * step);
        pushIndex(_blockSize - 1 - step, _blockSize - 1 - step);
        pushIndex(_blockSize - 1, _blockSize - 1 - 2 * step);
        pushIndex(_blockSize - 1, _blockSize - 1 - step);
        indices.push_back(RESTART_INDEX);

        pushIndex(_blockSize - 1 - 2 * step, _blockSize - 1 - step);
        pushIndex(_blockSize - 1 - 2 * step, _blockSize - 1);
        pushIndex(_blockSize - 1 - step, _blockSize - 1 - step);
        pushIndex(_blockSize - 1, _blockSize - 1);
        pushIndex(_blockSize - 1, _blockSize - 1 - step);
        indices.push_back(RESTART_INDEX);

        count += 11;

    } else { /* bitmask is _0_0*/
        /*
         *       *- - -*
         *       |    /|
         *       |  /  |
         *       |/    |
         * *- - -*- - -*
         * |    /|    /|
         * |  /  |  /  |
         * |/    |/    |
         * *- - -*- - -*
         *
         */

        pushIndex(_blockSize - 1 - step, _blockSize - 1 - 2 * step);
        pushIndex(_blockSize - 1 - step, _blockSize - 1 - step);
        pushIndex(_blockSize - 1, _blockSize - 1 - 2 * step);
        pushIndex(_blockSize - 1, _blockSize - 1 - step);
        indices.push_back(RESTART_INDEX);

        // TODO
        pushIndex(_blockSize - 1, _blockSize - 1);
        pushIndex(_blockSize - 1, _blockSize - 1 - step);
        pushIndex(_blockSize - 1 - step, _blockSize - 1);
        pushIndex(_blockSize - 1 - step, _blockSize - 1 - step);
        pushIndex(_blockSize - 1 - 2 * step, _blockSize - 1);
        pushIndex(_blockSize - 1 - 2 * step, _blockSize - 1 - step);
        indices.push_back(RESTART_INDEX);

        count += 12;
    }

    return count;
}

unsigned GeoMipMapping::loadBottomLeftCorner(unsigned step, unsigned configuration)
{
    unsigned count = 0;

    if ((configuration & LEFT_BORDER_BITMASK) && (configuration & BOTTOM_BORDER_BITMASK)) { /* bitmask is 1__1 */
        /*
         * *- - -*
         * |\    |
         * |  \  |
         * |    \|
         * *     *- - -*
         * |    / \    |
         * |  /     \  |
         * |/         \|
         * *- - -*- - -*
         *
         */

        pushIndex(step, _blockSize - 1 - 2 * step);
        pushIndex(0, _blockSize - 1 - 2 * step);
        pushIndex(step, _blockSize - 1 - step);
        pushIndex(0, _blockSize - 1);
        pushIndex(2 * step, _blockSize - 1);
        indices.push_back(RESTART_INDEX);

        pushIndex(step, _blockSize - 1 - step);
        pushIndex(2 * step, _blockSize - 1);
        pushIndex(2 * step, _blockSize - 1 - step);
        indices.push_back(RESTART_INDEX);

        count += 10;

    } else if (configuration & LEFT_BORDER_BITMASK) { /* bitmask is 1__0*/

        /*
         * *- - -*
         * |\    |
         * |  \  |
         * |    \|
         * *     *- - -*
         * |    /|    /|
         * |  /  |  /  |
         * |/    |/    |
         * *- - -*- - -*
         *
         */

        pushIndex(2 * step, _blockSize - 1);
        pushIndex(2 * step, _blockSize - 1 - step);
        pushIndex(step, _blockSize - 1);
        pushIndex(step, _blockSize - 1 - step);
        indices.push_back(RESTART_INDEX);

        pushIndex(step, _blockSize - 1 - 2 * step);
        pushIndex(0, _blockSize - 1 - 2 * step);
        pushIndex(step, _blockSize - 1 - step);
        pushIndex(0, _blockSize - 1);
        pushIndex(step, _blockSize - 1);
        indices.push_back(RESTART_INDEX);

        count += 11;

    } else if (configuration & BOTTOM_BORDER_BITMASK) { /* bitmask is 0__1 */
        /*
         * *- - -*
         * |    /|
         * |  /  |
         * |/    |
         * *- - -*- - -*
         * |    / \    |
         * |  /     \  |
         * |/         \|
         * *- - -*- - -*
         *
         */

        pushIndex(0, _blockSize - 1 - step);
        pushIndex(0, _blockSize - 1);
        pushIndex(step, _blockSize - 1 - step);
        pushIndex(2 * step, _blockSize - 1);
        pushIndex(2 * step, _blockSize - 1 - step);
        indices.push_back(RESTART_INDEX);

        pushIndex(0, _blockSize - 1 - 2 * step);
        pushIndex(0, _blockSize - 1 - step);
        pushIndex(step, _blockSize - 1 - 2 * step);
        pushIndex(step, _blockSize - 1 - step);
        indices.push_back(RESTART_INDEX);

        count += 11;

    } else { /* bitmask is 0__0*/
        /*
         * *- - -*
         * |    /|
         * |  /  |
         * |/    |
         * *- - -*- - -*
         * |    /|    /|
         * |  /  |  /  |
         * |/    |/    |
         * *- - -*- - -*
         *
         */

        pushIndex(2 * step, _blockSize - 1);
        pushIndex(2 * step, _blockSize - 1 - step);
        pushIndex(step, _blockSize - 1);
        pushIndex(step, _blockSize - 1 - step);
        pushIndex(0, _blockSize - 1);
        pushIndex(0, _blockSize - 1 - step);
        indices.push_back(RESTART_INDEX);

        pushIndex(0, _blockSize - 1 - 2 * step);
        pushIndex(0, _blockSize - 1 - step);
        pushIndex(step, _blockSize - 1 - 2 * step);
        pushIndex(step, _blockSize - 1 - step);
        indices.push_back(RESTART_INDEX);

        count += 12;
    }

    return count;
}

/**
 * @brief GeoMipMapping::loadTopBorder
 * @param block
 * @param step
 * @param configuration
 * @return
 */
unsigned GeoMipMapping::loadTopBorder(unsigned step, unsigned configuration)
{
    unsigned count = 0;
    if (configuration & TOP_BORDER_BITMASK) {

        /* Load border with crack avoidance */
        for (int j = step * 2; j < (int)(_blockSize) - (int)step * 3; j += step * 2) {
            pushIndex(j + 2 * step, step);
            pushIndex(j + 2 * step, 0);
            pushIndex(j + step, step);
            pushIndex(j, 0);
            pushIndex(j, step);
            indices.push_back(RESTART_INDEX);

            count += 6;
        }

    } else {
        /* Load border normally like any other block */
        for (int j = step * 2; j < (int)_blockSize - (int)step * 2; j += step) {
            pushIndex(j, 0);
            pushIndex(j, step);

            count += 2;
        }
    }
    indices.push_back(RESTART_INDEX);
    count++;

    return count;
}

/**
 * @brief GeoMipMapping::loadRightBorder
 * @param block
 * @param step
 * @param configuration
 * @return
 */
unsigned GeoMipMapping::loadRightBorder(unsigned step, unsigned configuration)
{
    unsigned count = 0;

    if (configuration & RIGHT_BORDER_BITMASK) {
        // TODO
        for (int i = step * 2; i < (int)(_blockSize) - (int)step * 3; i += step * 2) {

            pushIndex(_blockSize - 1 - step, i + 2 * step);
            pushIndex(_blockSize - 1, i + 2 * step);
            pushIndex(_blockSize - 1 - step, i + step);
            pushIndex(_blockSize - 1, i);
            pushIndex(_blockSize - 1 - step, i);
            indices.push_back(RESTART_INDEX);

            count += 6;
        }

    } else {
        /* Load border normally like any other block */
        for (int i = step * 2; i < (int)_blockSize - (int)step * 2; i += step) {
            pushIndex(_blockSize - 1, i);
            pushIndex(_blockSize - 1 - step, i);

            count += 2;
        }
    }

    indices.push_back(RESTART_INDEX);
    count++;

    return count;
}

/**
 * @brief GeoMipMapping::loadBottomBorder
 * @param block
 * @param step
 * @param configuration
 * @return
 */
unsigned GeoMipMapping::loadBottomBorder(unsigned step, unsigned configuration)
{
    unsigned count = 0;

    if (configuration & BOTTOM_BORDER_BITMASK) {
        for (int j = step * 2; j < (int)_blockSize - (int)step * 3; j += step * 2) {

            pushIndex(j, _blockSize - step - 1);
            pushIndex(j, _blockSize - 1);
            pushIndex(j + step, _blockSize - step - 1);
            pushIndex(j + 2 * step, _blockSize - 1);
            pushIndex(j + 2 * step, _blockSize - step - 1);
            indices.push_back(RESTART_INDEX);

            count += 6;
        }
    } else {
        /* Load border normally like any other block */
        for (int j = step * 2; j < (int)_blockSize - (int)step * 2; j += step) {
            pushIndex(j, _blockSize - 1 - step);
            pushIndex(j, _blockSize - 1);

            count += 2;
        }
    }
    indices.push_back(RESTART_INDEX);
    count++;

    return count;
}

/**
 * @brief GeoMipMapping::loadLeftBorder
 * @param block
 * @param step
 * @param configuration
 * @return
 */
unsigned GeoMipMapping::loadLeftBorder(unsigned step, unsigned configuration)
{
    unsigned count = 0;

    if (configuration & LEFT_BORDER_BITMASK) {
        for (int i = step * 2; i < (int)(_blockSize) - (int)(step * 3); i += step * 2) {

            pushIndex(step, i);
            pushIndex(0, i);
            pushIndex(step, i + step);
            pushIndex(0, i + 2 * step);
            pushIndex(step, i + 2 * step);
            indices.push_back(RESTART_INDEX);

            count += 6;
        }
    } else {
        /* Load border normally like any other block */
        for (int i = step * 2; i < (int)(_blockSize) - (int)(step * 2); i += step) {
            pushIndex(step, i);
            pushIndex(0, i);
            count += 2;
        }
    }
    indices.push_back(RESTART_INDEX);
    count++;

    return count;
}

/**
 * @brief Unloads the vertex array object, vertex buffer object and for each
 *        block the element buffer objects
 */
void GeoMipMapping::unloadBuffers()
{
    std::cout << "Unloading buffers" << std::endl;
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);

    AtlodUtil::checkGlError("GeoMipMapping deletion failed");
}
