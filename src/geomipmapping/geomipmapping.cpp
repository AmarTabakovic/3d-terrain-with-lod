#include "geomipmapping.h"
#include "../atlodutil.h"

#include <algorithm>
#include <cmath>

#include <chrono>

GeoMipMapping::GeoMipMapping(Heightmap heightmap, float xzScale, float yScale, unsigned blockSize, unsigned minLod, unsigned maxLod)
{
    std::cout << "Initialize GeoMipMapping" << std::endl;

    /* Min. LOD cannot be greater than max. LOD */
    if (minLod > maxLod) {
        std::cerr << "Error: Max LOD cannot be less than Min LOD" << std::endl;
        std::exit(1);
    }

    /* Check whether block size is of the form 2^n + 1*/
    if (((blockSize - 1) & (blockSize - 2)) != 0) {
        std::cerr << "Block size must be of the form 2^n + 1";
        std::exit(1);
    }

    _xzScale = xzScale;
    _yScale = yScale;
    _blockSize = blockSize;
    _heightmap = heightmap;
    _shader = Shader("../src/glsl/geomipmapping.vert", "../src/glsl/geomipmapping.frag");

    /* Always floor so that we do not "overshoot" when multiplying the number
     * of blocks with the block size */
    _nBlocksX = std::floor((heightmap.width() - 1) / (blockSize - 1));
    _nBlocksZ = std::floor((heightmap.height() - 1) / (blockSize - 1));

    /* Plus one is important */
    _width = _nBlocksX * (blockSize - 1) + 1;
    _height = _nBlocksZ * (blockSize - 1) + 1;

    /* Calculate maximum LOD level */
    _maxPossibleLod = std::log2(blockSize - 1);

    /* Calculate user defined LOD level bounds */
    _maxLod = std::min(maxLod, _maxPossibleLod);
    _minLod = std::max(0u, minLod);

    /* Set uniforms */
    shader().use();
    shader().setInt("texture1", 0);
    shader().setInt("heightmapTexture", 1);
    shader().setFloat("textureWidth", _heightmap.width());
    shader().setFloat("textureHeight", _heightmap.height());

    if (_hasTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _textureId);
    }

    loadBlocks();
}

GeoMipMapping::~GeoMipMapping()
{
    std::cout << "GeoMipMapping terrain destroyed" << std::endl;
}

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

            glm::vec3 temp = block.worldCenter - _lastCamera.position();
            float squaredDistance = glm::dot(temp, temp);

            if (!_lodActive)
                block.currentLod = _maxLod;
            else if (!_freezeCamera)
                block.currentLod = determineLodDistance(squaredDistance, _baseDistance, _doubleDistanceEachLevel);

            block.currentBorderBitmap = calculateBorderBitmap(block.blockId, j, i);
        }
    }

    glBindVertexArray(_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);

    /* Apply overlay texture (if existent) */
    if (_hasTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _textureId);
        shader().setFloat("doTexture", 1.0f);
    } else
        shader().setFloat("doTexture", 0.0f);

    /* Apply heightmap texture */
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _heightmap.heightmapTextureId());

    /* ============================== Second pass =============================
     * - For each block:
     *   - Check frustum culling
     *   - Set uniforms
     *   - Render center and border subblocks */
    for (unsigned i = 0; i < _nBlocksZ; i++) {
        for (unsigned j = 0; j < _nBlocksX; j++) {
            GeoMipMappingBlock& block = getBlock(j, i);

            /* Skip current block if not inside view frustum */
            bool intersects = _lastCamera.insideViewFrustum(block.p1, block.p2);
            if (_frustumCullingActive && !intersects)
                continue;

            float r = 0.3f, g = 0.3f, b = 0.3f;

            if (block.currentLod % 3 == 0)
                r = 0.7f;
            else if (block.currentLod % 3 == 1)
                g = 0.7f;
            else
                b = 0.7f;

            shader().setVec4("inColor", glm::vec4(r, g, b, 1.0f));
            shader().setVec2("offset", block.translation);

            unsigned currentIndex = block.currentLod - _minLod;

            /* First render the center subblocks (only for LOD >= 2, since
             * LOD 0 and 1 do not have a center block) */
            if (block.currentLod >= 2) {
                glDrawElements(GL_TRIANGLE_STRIP,
                    centerSizes[currentIndex],
                    GL_UNSIGNED_INT,
                    (void*)(centerStarts[currentIndex] * sizeof(unsigned)));
            }

            /* Then render the border subblocks
             * Note: This couild probably be optimized with
             * glMultiDrawElements() */
            currentIndex = currentIndex * 16 + block.currentBorderBitmap;
            glDrawElements(GL_TRIANGLE_STRIP,
                borderSizes[currentIndex],
                GL_UNSIGNED_INT,
                (void*)(borderStarts[currentIndex] * sizeof(unsigned)));
        }
    }

    AtlodUtil::checkGlError("GeoMipMapping render failed");
}

void GeoMipMapping::loadBlocks()
{
    glm::vec2 terrainCenter(_width / 2.0f, _height / 2.0f);

    for (unsigned i = 0; i < _nBlocksZ; i++) {
        for (unsigned j = 0; j < _nBlocksX; j++) {

            /* Determine min and max y-coordinates per block for the AABB */
            float minY = 9999999.0f, maxY = -9999999.0f;
            for (unsigned k = 0; k < _blockSize; k++) {
                for (unsigned l = 0; l < _blockSize; l++) {
                    float x = (j * (_blockSize - 1)) + l;
                    float z = (i * (_blockSize - 1)) + k;

                    float y = _heightmap.at(x, z);
                    minY = std::min(y, minY);
                    maxY = std::max(y, maxY);
                }
            }

            unsigned currentBlockId = i * _nBlocksX + j;

            float centerX = (j * (_blockSize - 1) + 0.5 * (_blockSize - 1));
            float centerZ = (i * (_blockSize - 1) + 0.5 * (_blockSize - 1));

            float trueY = _heightmap.at(centerX, centerZ) * _yScale;
            float aabbY = minY * _yScale + (((maxY - minY) * _yScale) / (2.0f * _yScale));

            glm::vec3 aabbCenter(((-(float)_width * _xzScale) / 2.0f) + centerX * _xzScale, aabbY, ((-(float)_height * _xzScale) / 2.0f) + centerZ * _xzScale);
            glm::vec3 blockCenter(aabbCenter.x, trueY, aabbCenter.z);

            glm::vec2 translation = glm::vec2(centerX, centerZ) - terrainCenter;

            glm::vec3 p1 = glm::vec3(aabbCenter.x - (_blockSize / 2.0f), aabbCenter.y - ((maxY - minY) / 2.0f), aabbCenter.z - (_blockSize / 2.0f));
            glm::vec3 p2 = glm::vec3(aabbCenter.x + (_blockSize / 2.0f), aabbCenter.y + ((maxY - minY) / 2.0f), aabbCenter.z + (_blockSize / 2.0f));

            GeoMipMappingBlock block = { currentBlockId, blockCenter, p1, p2, translation, 0, 0 };

            _blocks.push_back(block);
        }
    }
    std::cout << "Finished blocks" << std::endl;
}

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
    /* Disclaimer: some of the addition and subtraction below probably needs to be
     * refactored, since on some lines I directly subtract after I add, which
     * obviously makes no sense */
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
    }

    /* ============================= Load rest ==============================*/
    for (unsigned i = std::max(_minLod, 2u); i <= _maxLod; i++) {
        /* Load border subblocks */
        unsigned borderCount = loadBorderAreaForLod(i, totalCount);

        totalCount += borderCount;

        /* Load center subblocks */
        unsigned centerCount = loadCenterAreaForLod(i);
        totalCount += centerCount;
        centerStarts.push_back(totalCount - centerCount);
    }

    std::cout << "Allocated number of indices: " << indices.size() << std::endl;

    glGenBuffers(1, &_ebo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), &indices[0], GL_STATIC_DRAW);
}

/* I realized a bit too late that I can leave out the currentBlockId, since I can
 * calculate it with x and z anyway, but oh well */
unsigned GeoMipMapping::calculateBorderBitmap(unsigned currentBlockId, unsigned x, unsigned z)
{
    unsigned currentLod = _blocks[currentBlockId].currentLod;

    unsigned maxX = std::max((int)x - 1, 0);
    unsigned minX = std::min((int)x + 1, (int)_nBlocksX - 1);
    unsigned maxZ = std::max((int)z - 1, 0);
    unsigned minZ = std::min((int)z + 1, (int)_nBlocksZ - 1);

    GeoMipMappingBlock& leftBlock = getBlock(maxX, z);
    GeoMipMappingBlock& rightBlock = getBlock(minX, z);
    GeoMipMappingBlock& topBlock = getBlock(x, maxZ);
    GeoMipMappingBlock& bottomBlock = getBlock(x, minZ);

    unsigned leftLower = currentLod > leftBlock.currentLod ? 1 : 0;
    unsigned rightLower = currentLod > rightBlock.currentLod ? 1 : 0;
    unsigned topLower = currentLod > topBlock.currentLod ? 1 : 0;
    unsigned bottomLower = currentLod > bottomBlock.currentLod ? 1 : 0;

    return (leftLower << 3) | (rightLower << 2) | (topLower << 1) | bottomLower;
}

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
    return _minLod;
}

GeoMipMappingBlock& GeoMipMapping::getBlock(unsigned x, unsigned z)
{
    return _blocks[z * _nBlocksX + x];
}

void GeoMipMapping::pushIndex(unsigned x, unsigned y)
{
    indices.push_back(y * _blockSize + x);
}

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

unsigned GeoMipMapping::loadBorderAreaForLod(unsigned lod, unsigned accumulatedCount)
{
    unsigned totalCount = 0;

    /* 2^4 = 16 possible combinations */
    for (int i = 0; i < 16; i++) {
        unsigned count = loadBorderAreaForPermutation(lod, i);
        totalCount += count;
        accumulatedCount += count;
        borderStarts.push_back(accumulatedCount - count);
    }

    return totalCount;
}

unsigned GeoMipMapping::loadLod0Block()
{
    pushIndex(0, 0);
    pushIndex(0, _blockSize - 1);
    pushIndex(_blockSize - 1, 0);
    pushIndex(_blockSize - 1, _blockSize - 1);
    indices.push_back(RESTART_INDEX);

    return 5;
}

unsigned GeoMipMapping::loadLod1Block(unsigned permutation)
{
    unsigned count = 0;
    unsigned step = std::pow(2, _maxLod - 1);

    /* ============== Block is surrounded by lower LOD blocks ============== */
    if (permutation == 0b1111) {
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
    else if (permutation == 0b1110 || permutation == 0b1101 || permutation == 0b1011 || permutation == 0b00111) {
        /* Left or bottom block has the same LOD */
        if (permutation == 0b1110 || permutation == 0b0111) {

            pushIndex(0, 0);
            pushIndex(step, step);
            pushIndex(_blockSize - 1, 0);
            pushIndex(_blockSize - 1, _blockSize - 1);
            indices.push_back(RESTART_INDEX);
            count += 5;

            /* Bottom block has the same LOD*/
            if (permutation == 0b1110) {

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
            if (permutation == 0b1101) {

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
    else if (permutation == 0b0011 || permutation == 0b1100) {
        if (permutation == 0b0011) {

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
    else if (!(permutation & (LEFT_BORDER_BITMASK | TOP_BORDER_BITMASK))) {
        count += loadBottomRightCorner(step, permutation);

        pushIndex(0, 0);
        pushIndex(0, step);
        pushIndex(step, 0);
        pushIndex(step, step);
        indices.push_back(RESTART_INDEX);
        count += 5;
    } else if (!(permutation & (TOP_BORDER_BITMASK | RIGHT_BORDER_BITMASK))) {
        count += loadBottomLeftCorner(step, permutation);

        pushIndex(step, 0);
        pushIndex(step, step);
        pushIndex(_blockSize - 1, 0);
        pushIndex(_blockSize - 1, step);
        indices.push_back(RESTART_INDEX);
        count += 5;

    } else if (!(permutation & (RIGHT_BORDER_BITMASK | BOTTOM_BORDER_BITMASK))) {
        count += loadTopLeftCorner(step, permutation);

        pushIndex(step, step);
        pushIndex(step, _blockSize - 1);
        pushIndex(_blockSize - 1, step);
        pushIndex(_blockSize - 1, _blockSize - 1);
        indices.push_back(RESTART_INDEX);
        count += 5;
    } else if (!(permutation & (BOTTOM_BORDER_BITMASK | LEFT_BORDER_BITMASK))) {
        count += loadTopRightCorner(step, permutation);

        pushIndex(0, step);
        pushIndex(0, _blockSize - 1);
        pushIndex(step, step);
        pushIndex(step, _blockSize - 1);
        indices.push_back(RESTART_INDEX);
        count += 5;
    }

    return count;
}

unsigned GeoMipMapping::loadBorderAreaForPermutation(unsigned lod, unsigned permutation)
{
    unsigned step = std::pow(2, _maxLod - lod);
    unsigned count = 0;

    count += loadTopLeftCorner(step, permutation);
    count += loadTopBorder(step, permutation);
    count += loadTopRightCorner(step, permutation);
    count += loadRightBorder(step, permutation);
    count += loadBottomRightCorner(step, permutation);
    count += loadBottomBorder(step, permutation);
    count += loadBottomLeftCorner(step, permutation);
    count += loadLeftBorder(step, permutation);

    borderSizes.push_back(count);

    return count;
}

unsigned GeoMipMapping::loadTopLeftCorner(unsigned step, unsigned permutation)
{
    unsigned count = 0;
    if ((permutation & LEFT_BORDER_BITMASK) && (permutation & TOP_BORDER_BITMASK)) { /* bitmask is 1_1_ */
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

    } else if (permutation & LEFT_BORDER_BITMASK) { /* bitmask is 1_0_*/

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

    } else if (permutation & TOP_BORDER_BITMASK) { /* bitmask is 0_1_ */
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

unsigned GeoMipMapping::loadTopRightCorner(unsigned step, unsigned permutation)
{
    unsigned count = 0;
    if ((permutation & RIGHT_BORDER_BITMASK) && (permutation & TOP_BORDER_BITMASK)) { /* bitmask is _11_ */
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

    } else if (permutation & RIGHT_BORDER_BITMASK) { /* bitmask is _10_*/

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

    } else if (permutation & TOP_BORDER_BITMASK) { /* bitmask is _01_ */
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

unsigned GeoMipMapping::loadBottomRightCorner(unsigned step, unsigned permutation)
{
    unsigned count = 0;

    if ((permutation & RIGHT_BORDER_BITMASK) && (permutation & BOTTOM_BORDER_BITMASK)) { /* bitmask is _1_1 */
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

    } else if (permutation & RIGHT_BORDER_BITMASK) { /* bitmask is _1_0*/

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

    } else if (permutation & BOTTOM_BORDER_BITMASK) { /* bitmask is _0_1 */
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

unsigned GeoMipMapping::loadBottomLeftCorner(unsigned step, unsigned permutation)
{
    unsigned count = 0;

    if ((permutation & LEFT_BORDER_BITMASK) && (permutation & BOTTOM_BORDER_BITMASK)) { /* bitmask is 1__1 */
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

    } else if (permutation & LEFT_BORDER_BITMASK) { /* bitmask is 1__0*/

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

    } else if (permutation & BOTTOM_BORDER_BITMASK) { /* bitmask is 0__1 */
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

unsigned GeoMipMapping::loadTopBorder(unsigned step, unsigned permutation)
{
    unsigned count = 0;
    if (permutation & TOP_BORDER_BITMASK) {

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

unsigned GeoMipMapping::loadRightBorder(unsigned step, unsigned permutation)
{
    unsigned count = 0;

    if (permutation & RIGHT_BORDER_BITMASK) {

        /* Load border with crack avoidance */
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
        /* Left and right borders are loaded specially due to CCW orientation */
        pushIndex(_blockSize - 1 - step, 3 * step);
        pushIndex(_blockSize - 1, 2 * step);
        pushIndex(_blockSize - 1 - step, 2 * step);
        indices.push_back(RESTART_INDEX);

        count += 4;

        for (int i = step * 2; i < (int)_blockSize - (int)step * 2 - 1; i += step) {
            pushIndex(_blockSize - 1, i);
            pushIndex(_blockSize - 1 - step, i + step);

            count += 2;
        }

        pushIndex(_blockSize - 1, (int)_blockSize - (int)step * 2 - 1);
        count++;
    }

    indices.push_back(RESTART_INDEX);
    count++;

    return count;
}

unsigned GeoMipMapping::loadBottomBorder(unsigned step, unsigned permutation)
{
    unsigned count = 0;

    if (permutation & BOTTOM_BORDER_BITMASK) {

        /* Load border with crack avoidance */
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

unsigned GeoMipMapping::loadLeftBorder(unsigned step, unsigned permutation)
{
    unsigned count = 0;

    if (permutation & LEFT_BORDER_BITMASK) {
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
        /* Left and right borders are loaded specially due to CCW orientation */
        pushIndex(0, step * 3);
        pushIndex(step, step * 2);
        pushIndex(0, step * 2);
        indices.push_back(RESTART_INDEX);

        count += 4;

        for (int i = step * 2; i < (int)_blockSize - (int)step * 2 - 1; i += step) {
            pushIndex(step, i);
            pushIndex(0, i + step);
            count += 2;
        }

        pushIndex(step, (int)_blockSize - (int)step * 2 - 1);
        count++;
    }
    indices.push_back(RESTART_INDEX);
    count++;

    return count;
}

void GeoMipMapping::unloadBuffers()
{
    std::cout << "Unloading buffers" << std::endl;
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);

    AtlodUtil::checkGlError("GeoMipMapping deletion failed");
}

unsigned GeoMipMapping::nBlocksX()
{
    return _nBlocksX;
}

unsigned GeoMipMapping::nBlocksZ()
{
    return _nBlocksZ;
}

bool GeoMipMapping::freezeCamera()
{
    return _freezeCamera;
}

bool GeoMipMapping::lodActive()
{
    return _lodActive;
}

bool GeoMipMapping::frustumCullingActive()
{
    return _frustumCullingActive;
}

void GeoMipMapping::freezeCamera(bool freezeCamera)
{
    _freezeCamera = freezeCamera;
}

void GeoMipMapping::lodActive(bool lodActive)
{
    _lodActive = lodActive;
}

void GeoMipMapping::frustumCullingActive(bool frustumCullingActive)
{
    _frustumCullingActive = frustumCullingActive;
}

void GeoMipMapping::baseDistance(float baseDistance)
{
    _baseDistance = baseDistance;
}

void GeoMipMapping::doubleDistanceEachLevel(bool doubleDistanceEachLevel)
{
    _doubleDistanceEachLevel = doubleDistanceEachLevel;
}
