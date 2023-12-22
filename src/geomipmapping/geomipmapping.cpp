#include "geomipmapping.h"
#include "../atlodutil.h"

#include <algorithm>
#include <cmath>

#include <chrono>

GeoMipMapping::GeoMipMapping(Heightmap heightmap, float xzScale, float yScale, unsigned blockSize)
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
    this->_width = _nBlocksX * (blockSize - 1) + 1;
    this->_height = _nBlocksZ * (blockSize - 1) + 1;

    /* Calculate maximum LOD level */
    _maxLod = std::log2(blockSize - 1);

    for (unsigned i = 0; i < _nBlocksZ; i++) {
        for (unsigned j = 0; j < _nBlocksX; j++) {
            unsigned currentBlock = i * _nBlocksX + j;
            unsigned startIndex = i * _width * (blockSize - 1) + (j * (blockSize - 1));

            float x = (j * (_blockSize - 1) + 0.5 * (_blockSize - 1));
            float z = (i * (_blockSize - 1) + 0.5 * (_blockSize - 1));

            float y = heightmap.at(x, z);
            glm::vec3 blockCenter(((-(int)_width * _xzScale) / 2) + x * _xzScale, y * _yScale, ((-(int)_height * _xzScale) / 2) + z * _xzScale);

            GeoMipMappingBlock block = GeoMipMappingBlock(currentBlock, startIndex, blockCenter, _blockSize);
            block.terrain = this;

            _blocks.push_back(block);
        }
    }
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
 * @brief Updates and renders the entire GeoMipMapping terrain.
 *
 * This method is called each frame.
 *
 * @param camera - the viewing camera
 */
void GeoMipMapping::render(Camera camera)
{
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RESTART_INDEX);

    if (!camera.frozen)
        _lastCamera = camera;

    /* ================================ First pass ===============================
     * - For each block:
     *   - Check and update the distance to camera
     *   - Update the LOD level
     *   - Update the neighborhood border bitmap */
    for (unsigned i = 0; i < _nBlocksZ; i++) {
        for (unsigned j = 0; j < _nBlocksX; j++) {
            GeoMipMappingBlock& block = getBlock(j, i);

            glm::vec3 temp = block._center - _lastCamera.position();
            float squaredDistance = glm::dot(temp, temp);

            block._squaredDistanceToCamera = squaredDistance;
            block._currentLod = determineLodDistance(squaredDistance, _baseDistance, false /*true */);
            block._currentBorderBitmap = calculateBorderBitmap(block._blockId, j, i);
        }
    }

    if (_hasTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _textureId);
        shader().setFloat("doTexture", 1.0f);
    } else
        shader().setFloat("doTexture", 0.0f);

    glBindVertexArray(_vao);

    /* ============================== Second pass =============================
     * - For each block:
     *   - Check frustum culling
     *   - Render center and border subblocks
     */
    for (unsigned i = 0; i < _nBlocksZ; i++) {
        for (unsigned j = 0; j < _nBlocksX; j++) {
            GeoMipMappingBlock& block = getBlock(j, i);

            /* TODO:
             * - Collect visible blocks in first pass
             * - Quadtrees */
            if (!block.insideViewFrustum(_lastCamera))
                continue;

            unsigned currentIndex = block._currentLod;

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, block._ebo);

            float r = 0.3f, g = 0.3f, b = 0.3f;

            if (block._currentLod % 3 == 0)
                r = 0.7f;
            else if (block._currentLod % 3 == 1)
                g = 0.7f;
            else
                b = 0.7f;

            shader().setVec4("inColor", glm::vec4(r, g, b, 1.0f));

            /* First render the center subblocks (only for LOD >= 2, since
             * LOD 0 and 1 do not have a center block) */
            if (block._currentLod >= 2) {
                glDrawElements(GL_TRIANGLE_STRIP,
                    block.centerSizes[currentIndex],
                    GL_UNSIGNED_INT,
                    (void*)(block.centerStarts[currentIndex] * sizeof(unsigned)));
            }

            /* Then render the border subblocks
             * Note: This couild probably be optimized with
             * glMultiDrawElements() */
            currentIndex = currentIndex * 16 + block._currentBorderBitmap;
            glDrawElements(GL_TRIANGLE_STRIP,
                block.borderSizes[currentIndex],
                GL_UNSIGNED_INT,
                (void*)(block.borderStarts[currentIndex] * sizeof(unsigned)));
        }
    }

    AtlodUtil::checkGlError("GeoMipMapping render failed");
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
    for (int i = 0; i < _maxLod; i++) {
        if (distance < distancePower * distancePower * baseDist * baseDist)
            return _maxLod - i;

        if (doubleEachLevel)
            distancePower <<= 1;
        else
            distancePower++;
    }
    return 0;
}

/**
 * @brief Detemrines the LOD level using the approach mentioned in the
 *        original GeoMipMapping paper.
 * @param distance
 * @return
 */
unsigned GeoMipMapping::determineLodPaper(float distance)
{
    return 0;
}

/**
 * @brief GeoMipMapping::loadBuffers
 *
 * This method should be called once before entering the main rendering loop.
 *
 * TODO: When loading indices, calculate deltas for LOD selection
 */
void GeoMipMapping::loadBuffers()
{
    //int signedHeight = (int)_height;
    //int signedWidth = (int)_width;
    int signedWidth = (int)_heightmap.width();
    int signedHeight = (int)_heightmap.height();

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    loadIndices();

    loadNormals();

    /* Set up vertex buffer */

    /* In order to load vertices centered around the zero point,
     * TODO
     */
    int widthDiff = _heightmap.width() - _width;
    int heightDiff = _heightmap.height() - _height;

    int padXBegin, padXEnd, padYBegin, padYEnd;

    padYBegin = std::floor((float)heightDiff / 2.0f);
    padXBegin = std::floor((float)widthDiff / 2.0f);
    padYEnd = padYBegin;
    padXEnd = padXBegin;

    if (widthDiff % 2 != 0) /* Width difference is odd */
        padXEnd++;

    if (heightDiff % 2 != 0) /* Height difference is odd */
        padYEnd++;

    for (unsigned i = padYBegin; i < _heightmap.height() - padYEnd; i++) {
        for (unsigned j = padXBegin; j < _heightmap.width() - padXEnd; j++) {
            float y = _heightmap.at(j, i);

            /* Load vertices around center point
             * TODO: Maybe do not center here, but in the view matrix? */
            float x = (-signedWidth / 2.0f + signedWidth * j / (float)signedWidth);
            float z = (-signedHeight / 2.0f + signedHeight * i / (float)signedHeight);

            glm::vec3 normal = _normals[i * _width + j];

            _vertices.push_back(x); /* position x */
            _vertices.push_back(y); /* position y */
            _vertices.push_back(z); /* position z */
            _vertices.push_back(normal.x); /* normal x */
            _vertices.push_back(normal.y); /* normal y */
            _vertices.push_back(normal.z); /* normal z */
            _vertices.push_back((float)(j ) / (float)(_heightmap.width())); /* texture x */
            _vertices.push_back((float)(i ) /(float)(_heightmap.height())); /* texture y */
        }
    }

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(float), &_vertices[0], GL_STATIC_DRAW);

    /* Position attribute */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    /* Normal attribute */
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    /* Texture attribute */
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    /* Vertices, indces and normals were loaded to the GPU, clear them */
    _normals.clear();
    _normals.shrink_to_fit();
    _vertices.clear();
    _vertices.shrink_to_fit();
    for (auto& block : _blocks) {
        block.indices.clear();
        block.indices.shrink_to_fit();
    }

    AtlodUtil::checkGlError("GeoMipMapping load failed");
}

/**
 * @brief Helper function for calculating and adding the normals
 *        of a triangle face.
 * @param j
 * @param i
 * @param isBottomRight
 */
void GeoMipMapping::calculateNormal(unsigned j, unsigned i, bool isBottomRight)
{
    int signedHeight = (int)_height;
    int signedWidth = (int)_width;

    int offset = 1;
    if (isBottomRight) {
        j++;
        i++;
        offset = -1;
    }

    float y = _heightmap.at(j, i);
    float x = (-signedWidth / 2.0f + signedWidth * j / (float)signedWidth);
    float z = (-signedHeight / 2.0f + signedHeight * i / (float)signedHeight);

    glm::vec3 v0(x, y, z);

    float y1 = _heightmap.at(j + offset, i);
    float x1 = (-signedWidth / 2.0f + signedWidth * (j + offset) / (float)signedWidth);
    float z1 = z;

    float y2 = _heightmap.at(j, i + offset);
    float x2 = x;
    float z2 = (-signedHeight / 2.0f + signedHeight * (i + offset) / (float)signedHeight);

    glm::vec3 v1(x1, y1, z1);
    glm::vec3 v2(x2, y2, z2);

    glm::vec3 a = v1 - v0;
    glm::vec3 b = v2 - v0;

    glm::vec3 normal = glm::cross(b, a);

    _normals[i * _width + j] += normal;
    _normals[i * _width + j + offset] += normal;
    _normals[(i + offset) * _width + j] += normal;
}

/**
 * @brief GeoMipMapping::loadNormals
 *
 * This normal calculating method is based on SLProject's calcNormals() method.
 * SLProject is developed at the Bern University of Applied Sciences.
 *
 * TODO: Define method in Terain superclass
 */
void GeoMipMapping::loadNormals()
{

    _normals.resize(_height * _width);

    /* TODO: Refactor the below into a method i.e. constructor? */
    unsigned widthDiff = _heightmap.width() - _width;
    unsigned heightDiff = _heightmap.height() - _height;

    unsigned padXBegin, padXEnd, padYBegin, padYEnd;

    padYBegin = std::floor((float)heightDiff / 2.0f);
    padXBegin = std::floor((float)widthDiff / 2.0f);
    padYEnd = padYBegin;
    padXEnd = padXBegin;

    if (widthDiff % 2 != 0) /* Width difference is odd */
        padXEnd++;

    if (heightDiff % 2 != 0) /* Height difference is odd */
        padYEnd++;

    for (unsigned i = 0; i < _height - 1; i++) {
        for (unsigned j = 0; j < _width - 1; j++) {
            _normals.push_back(glm::vec3(0.0f));
        }
    }

    /* Load normals for every vertex */
    //    for (unsigned i = 0; i < _height - 1; i++) {
    //        for (unsigned j = 0; j < _width - 1; j++) {
    //            calculateNormal(j, i, false);
    //            calculateNormal(j, i, true);
    //        }
    //    }

    for (unsigned i = padYBegin; i < _heightmap.height() - padYEnd - 1; i++) {
        for (unsigned j = padXBegin; j < _heightmap.width() - padXEnd - 1; j++) {
            calculateNormal(j, i, false);
            calculateNormal(j, i, true);
        }
    }

    for (int i = 0; i < _normals.size(); i++) {
        _normals[i] = glm::normalize(_normals[i]);
    }
}

/**
 * @brief Loads the indices for each block.
 */
void GeoMipMapping::loadIndices()
{
    for (unsigned i = 0; i < _nBlocksZ; i++) {
        for (unsigned j = 0; j < _nBlocksX; j++) {
            unsigned currentBlock = i * _nBlocksX + j;

            loadGeoMipMapsForBlock(_blocks[currentBlock]);
        }
    }
}

/**
 * @brief GeoMipMapping::loadGeoMipMapsForBlock
 * @param block
 */
void GeoMipMapping::loadGeoMipMapsForBlock(GeoMipMappingBlock& block)
{
    unsigned totalCount = 0;

    if (_nIndicesPerBlock != 0)
        block.indices.reserve(_nIndicesPerBlock);

    /* ========================== Load LOD 0 block ==========================*/
    unsigned lod0Count = loadLod0Block(block);
    totalCount += lod0Count;

    /* For LOD 0, the (single) border block is always the same */
    for (int i = 0; i < 16; i++) {
        block.borderStarts.push_back(totalCount - lod0Count);
        block.borderSizes.push_back(lod0Count);
    }

    /* LOD 0 border block does not have a center */
    block.centerStarts.push_back(0);
    block.centerSizes.push_back(0);
    block.geoMipMaps.push_back(GeoMipMap(0));

    /* ========================== Load LOD 1 block ==========================*/
    for (unsigned i = 0; i < 16; i++) {
        unsigned lod1Count = loadLod1Block(block, i);
        totalCount += lod1Count;
        block.borderStarts.push_back(totalCount - lod1Count);
        block.borderSizes.push_back(lod1Count);
    }

    /* LOD 1 border block does not have a center */
    block.centerStarts.push_back(0);
    block.centerSizes.push_back(0);
    block.geoMipMaps.push_back(GeoMipMap(1));

    /* ============================= Load rest ==============================*/
    for (unsigned i = 2; i <= _maxLod; i++) {
        /* Load border subblocks */
        unsigned borderCount = loadBorderAreaForLod(block,
            block._startIndex, i,
            totalCount);

        totalCount += borderCount;

        /* Load center subblocks */
        unsigned centerCount = loadCenterAreaForLod(block, block._startIndex, i);
        totalCount += centerCount;

        block.centerStarts.push_back(totalCount - centerCount);

        block.geoMipMaps.push_back(GeoMipMap(i));
    }

    if (_nIndicesPerBlock == 0)
        _nIndicesPerBlock = block.indices.size();

    glGenBuffers(1, &block._ebo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, block._ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, block.indices.size() * sizeof(unsigned), &block.indices[0], GL_STATIC_DRAW);
}

/**
 * @brief GeoMipMapping::loadCenterAreaForLod
 * @param block
 * @param startIndex
 * @param lod the current LOD levell
 * @return the total number of indices loaded
 */
unsigned GeoMipMapping::loadCenterAreaForLod(GeoMipMappingBlock& block, unsigned startIndex, unsigned lod)
{
    unsigned step = std::pow(2, _maxLod - lod);

    unsigned count = 0;

    for (unsigned i = step; i < _blockSize - step - 1; i += step) {
        for (unsigned j = step; j < _blockSize - step; j += step) {
            block.indices.push_back(block.getRelativeIndex(_width, j, i));
            block.indices.push_back(block.getRelativeIndex(_width, j, i + step));
            count += 2;
        }
        block.indices.push_back(RESTART_INDEX);
        count++;
    }

    block.centerSizes.push_back(count);

    return count;
}

/**
 * @brief GeoMipMapping::loadBorderAreaForLod
 * @param block
 * @param startIndex
 * @param lod
 * @return
 */
unsigned GeoMipMapping::loadBorderAreaForLod(GeoMipMappingBlock& block, unsigned startIndex, unsigned lod, unsigned accumulatedCount)
{
    unsigned totalCount = 0;

    /* 2^4 = 16 possible combinations */
    for (int i = 0; i < 16; i++) {
        unsigned count = loadBorderAreaForConfiguration(block, startIndex, lod, i);
        totalCount += count;
        accumulatedCount += count;
        block.borderStarts.push_back(accumulatedCount - count);
    }

    return totalCount;
}

/**
 * @brief GeoMipMapping::load2x2Block
 * @return
 */
unsigned GeoMipMapping::loadLod0Block(GeoMipMappingBlock& block)
{
    block.pushRelativeIndex(_width, 0, 0);
    block.pushRelativeIndex(_width, 0, _blockSize - 1);
    block.pushRelativeIndex(_width, _blockSize - 1, 0);
    block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1);
    block.indices.push_back(RESTART_INDEX);

    return 5;
}

/**
 * @brief Loads a single LOD 1 (i.e. 3x3) block.
 * @return Number of indices generated
 */
unsigned GeoMipMapping::loadLod1Block(GeoMipMappingBlock& block, unsigned configuration)
{
    // TODO
    unsigned count = 0;
    unsigned step = std::pow(2, _maxLod - 1);

    /* ============== Block is surrounded by lower LOD blocks ============== */
    if (configuration == 0b1111) {
        block.pushRelativeIndex(_width, 0, 0);
        block.pushRelativeIndex(_width, step, step);
        block.pushRelativeIndex(_width, _blockSize - 1, 0);
        block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1);
        block.indices.push_back(RESTART_INDEX);

        block.pushRelativeIndex(_width, 0, 0);
        block.pushRelativeIndex(_width, 0, _blockSize - 1);
        block.pushRelativeIndex(_width, step, step);
        block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1);
        block.indices.push_back(RESTART_INDEX);
        count += 10;
    }

    /* ======= Block is surrounded by exactly three lower LOD blocks ======= */
    else if (configuration == 0b1110 || configuration == 0b1101 || configuration == 0b1011 || configuration == 0b00111) {
        /* Left or bottom block has the same LOD */
        if (configuration == 0b1110 || configuration == 0b0111) {
            block.pushRelativeIndex(_width, 0, 0);
            block.pushRelativeIndex(_width, step, step);
            block.pushRelativeIndex(_width, _blockSize - 1, 0);
            block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1);
            block.indices.push_back(RESTART_INDEX);
            count += 5;

            /* Bottom block has the same LOD*/
            if (configuration == 0b1110) {
                block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1);
                block.pushRelativeIndex(_width, step, step);
                block.pushRelativeIndex(_width, step, _blockSize - 1);
                block.pushRelativeIndex(_width, 0, _blockSize - 1);
                block.indices.push_back(RESTART_INDEX);

                block.pushRelativeIndex(_width, 0, _blockSize - 1);
                block.pushRelativeIndex(_width, step, step);
                block.pushRelativeIndex(_width, 0, 0);
                block.indices.push_back(RESTART_INDEX);

                count += 9;
            } else { /* Left block has the same LOD */
                block.pushRelativeIndex(_width, 0, _blockSize - 1);
                block.pushRelativeIndex(_width, step, step);
                block.pushRelativeIndex(_width, 0, step);
                block.pushRelativeIndex(_width, 0, 0);
                block.indices.push_back(RESTART_INDEX);

                block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1);
                block.pushRelativeIndex(_width, step, step);
                block.pushRelativeIndex(_width, 0, _blockSize - 1);
                block.indices.push_back(RESTART_INDEX);

                count += 9;
            }

        } else { /* Top or right block has the same LOD */
            block.pushRelativeIndex(_width, 0, 0);
            block.pushRelativeIndex(_width, 0, _blockSize - 1);
            block.pushRelativeIndex(_width, step, step);
            block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1);
            block.indices.push_back(RESTART_INDEX);

            count += 5;

            /* Top block has the same LOD*/
            if (configuration == 0b1101) {
                block.pushRelativeIndex(_width, 0, 0);
                block.pushRelativeIndex(_width, step, step);
                block.pushRelativeIndex(_width, step, 0);
                block.pushRelativeIndex(_width, _blockSize - 1, 0);
                block.indices.push_back(RESTART_INDEX);

                block.pushRelativeIndex(_width, step, step);
                block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1);
                block.pushRelativeIndex(_width, _blockSize - 1, 0);
                block.indices.push_back(RESTART_INDEX);

                count += 9;

            } else { /* Right block has the same LOD */

                block.pushRelativeIndex(_width, _blockSize - 1, 0);
                block.pushRelativeIndex(_width, step, step);
                block.pushRelativeIndex(_width, _blockSize - 1, step);
                block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1);
                block.indices.push_back(RESTART_INDEX);

                block.pushRelativeIndex(_width, 0, 0);
                block.pushRelativeIndex(_width, step, step);
                block.pushRelativeIndex(_width, _blockSize - 1, 0);
                block.indices.push_back(RESTART_INDEX);

                count += 9;
            }
        }
    }

    /* ======== Block is surrounded by exaclty two lower LOD blocks ======== */
    else if (configuration == 0b0011 || configuration == 0b1100) {
        if (configuration == 0b0011) {
            block.pushRelativeIndex(_width, _blockSize - 1, step);
            block.pushRelativeIndex(_width, _blockSize - 1, 0);
            block.pushRelativeIndex(_width, step, step);
            block.pushRelativeIndex(_width, 0, 0);
            block.pushRelativeIndex(_width, 0, _blockSize - 1);
            block.indices.push_back(RESTART_INDEX);

            block.pushRelativeIndex(_width, 0, step);
            block.pushRelativeIndex(_width, 0, _blockSize - 1);
            block.pushRelativeIndex(_width, step, step);
            block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1);
            block.pushRelativeIndex(_width, _blockSize - 1, step);
            block.indices.push_back(RESTART_INDEX);

            count += 12;
        } else {
            block.pushRelativeIndex(_width, step, 0);
            block.pushRelativeIndex(_width, 0, 0);
            block.pushRelativeIndex(_width, step, step);
            block.pushRelativeIndex(_width, 0, _blockSize - 1);
            block.pushRelativeIndex(_width, step, _blockSize - 1);
            block.indices.push_back(RESTART_INDEX);

            block.pushRelativeIndex(_width, step, _blockSize - 1);
            block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1);
            block.pushRelativeIndex(_width, step, step);
            block.pushRelativeIndex(_width, _blockSize - 1, 0);
            block.pushRelativeIndex(_width, step, 0);
            block.indices.push_back(RESTART_INDEX);

            count += 12;
        }
    }

    /*
     * ==== Determine which corner is a regular quad and the method of the ====
     *      opposing corner
     */
    else if (!(configuration & (LEFT_BORDER_BITMASK | TOP_BORDER_BITMASK))) {
        count += loadBottomRightCorner(block, step, configuration);
        block.pushRelativeIndex(_width, 0, 0);
        block.pushRelativeIndex(_width, 0, step);
        block.pushRelativeIndex(_width, step, 0);
        block.pushRelativeIndex(_width, step, step);

        block.indices.push_back(RESTART_INDEX);
        count += 5;
    } else if (!(configuration & (TOP_BORDER_BITMASK | RIGHT_BORDER_BITMASK))) {
        count += loadBottomLeftCorner(block, step, configuration);
        block.pushRelativeIndex(_width, step, 0);
        block.pushRelativeIndex(_width, step, step);
        block.pushRelativeIndex(_width, _blockSize - 1, 0);
        block.pushRelativeIndex(_width, _blockSize - 1, step);

        block.indices.push_back(RESTART_INDEX);
        count += 5;

    } else if (!(configuration & (RIGHT_BORDER_BITMASK | BOTTOM_BORDER_BITMASK))) {
        count += loadTopLeftCorner(block, step, configuration);
        block.pushRelativeIndex(_width, step, step);
        block.pushRelativeIndex(_width, step, _blockSize - 1);
        block.pushRelativeIndex(_width, _blockSize - 1, step);
        block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1);

        block.indices.push_back(RESTART_INDEX);
        count += 5;
    } else if (!(configuration & (BOTTOM_BORDER_BITMASK | LEFT_BORDER_BITMASK))) {
        count += loadTopRightCorner(block, step, configuration);
        block.pushRelativeIndex(_width, 0, step);
        block.pushRelativeIndex(_width, 0, _blockSize - 1);
        block.pushRelativeIndex(_width, step, step);
        block.pushRelativeIndex(_width, step, _blockSize - 1);

        block.indices.push_back(RESTART_INDEX);
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
unsigned GeoMipMapping::loadBorderAreaForConfiguration(GeoMipMappingBlock& block, unsigned startIndex, unsigned lod, unsigned configuration)
{

    /* The idea is to traverse the border subblocks clockwise starting from
     * the top left corner.
     *
     * TODO:
     * - Refactor the below left, right, top, bottom cases into single methods,
     *   same for corners */

    unsigned step = std::pow(2, _maxLod - lod);
    unsigned count = 0;

    count += loadTopLeftCorner(block, step, configuration);
    count += loadTopBorder(block, step, configuration);
    count += loadTopRightCorner(block, step, configuration);
    count += loadRightBorder(block, step, configuration);
    count += loadBottomRightCorner(block, step, configuration);
    count += loadBottomBorder(block, step, configuration);
    count += loadBottomLeftCorner(block, step, configuration);
    count += loadLeftBorder(block, step, configuration);

    block.borderSizes.push_back(count);

    return count;
}

/**
 * @brief GeoMipMapping::loadTopLeftcorner
 * @param block
 * @param step
 * @param configuration
 * @return
 */
unsigned GeoMipMapping::loadTopLeftCorner(GeoMipMappingBlock& block, unsigned step, unsigned configuration)
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

        block.pushRelativeIndex(_width, 2 * step, step);
        block.pushRelativeIndex(_width, 2 * step, 0);
        block.pushRelativeIndex(_width, step, step);
        block.pushRelativeIndex(_width, 0, 0);
        block.pushRelativeIndex(_width, 0, 2 * step);
        block.indices.push_back(RESTART_INDEX);

        block.pushRelativeIndex(_width, step, 2 * step);
        block.pushRelativeIndex(_width, step, step);
        block.pushRelativeIndex(_width, 0, 2 * step);
        block.indices.push_back(RESTART_INDEX);

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

        block.pushRelativeIndex(_width, step, 0);
        block.pushRelativeIndex(_width, 0, 0);
        block.pushRelativeIndex(_width, step, step);
        block.pushRelativeIndex(_width, 0, 2 * step);
        block.pushRelativeIndex(_width, step, 2 * step);
        block.indices.push_back(RESTART_INDEX);

        block.pushRelativeIndex(_width, step, 0);
        block.pushRelativeIndex(_width, step, step);
        block.pushRelativeIndex(_width, 2 * step, 0);
        block.pushRelativeIndex(_width, 2 * step, step);
        block.indices.push_back(RESTART_INDEX);

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

        block.pushRelativeIndex(_width, 0, step);
        block.pushRelativeIndex(_width, 0, 2 * step);
        block.pushRelativeIndex(_width, step, step);
        block.pushRelativeIndex(_width, step, 2 * step);
        block.indices.push_back(RESTART_INDEX);

        block.pushRelativeIndex(_width, 2 * step, step);
        block.pushRelativeIndex(_width, 2 * step, 0);
        block.pushRelativeIndex(_width, step, step);
        block.pushRelativeIndex(_width, 0, 0);
        block.pushRelativeIndex(_width, 0, step);
        block.indices.push_back(RESTART_INDEX);

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

        block.pushRelativeIndex(_width, 0, step);
        block.pushRelativeIndex(_width, 0, 2 * step);
        block.pushRelativeIndex(_width, step, step);
        block.pushRelativeIndex(_width, step, 2 * step);
        block.indices.push_back(RESTART_INDEX);

        block.pushRelativeIndex(_width, 0, 0);
        block.pushRelativeIndex(_width, 0, step);
        block.pushRelativeIndex(_width, step, 0);
        block.pushRelativeIndex(_width, step, step);
        block.pushRelativeIndex(_width, 2 * step, 0);
        block.pushRelativeIndex(_width, 2 * step, step);
        block.indices.push_back(RESTART_INDEX);

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
unsigned GeoMipMapping::loadTopRightCorner(GeoMipMappingBlock& block, unsigned step, unsigned configuration)
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

        block.pushRelativeIndex(_width, _blockSize - 1 - step, 2 * step);
        block.pushRelativeIndex(_width, _blockSize - 1, 2 * step);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, step);
        block.pushRelativeIndex(_width, _blockSize - 1, 0);
        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, 0);
        block.indices.push_back(RESTART_INDEX);

        block.pushRelativeIndex(_width, _blockSize - 1 - step, step);
        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, 0);
        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, step);
        block.indices.push_back(RESTART_INDEX);

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

        block.pushRelativeIndex(_width, _blockSize - 1 - step, 2 * step);
        block.pushRelativeIndex(_width, _blockSize - 1, 2 * step);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, step);
        block.pushRelativeIndex(_width, _blockSize - 1, 0);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, 0);
        block.indices.push_back(RESTART_INDEX);

        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, 0);
        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, step);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, 0);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, step);
        block.indices.push_back(RESTART_INDEX);

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

        block.pushRelativeIndex(_width, _blockSize - 1 - step, step);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, 2 * step);
        block.pushRelativeIndex(_width, _blockSize - 1, step);
        block.pushRelativeIndex(_width, _blockSize - 1, 2 * step);
        block.indices.push_back(RESTART_INDEX);

        block.pushRelativeIndex(_width, _blockSize - 1, step);
        block.pushRelativeIndex(_width, _blockSize - 1, 0);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, step);
        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, 0);
        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, step);
        block.indices.push_back(RESTART_INDEX);

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

        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, 0);
        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, step);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, 0);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, step);
        block.pushRelativeIndex(_width, _blockSize - 1, 0);
        block.pushRelativeIndex(_width, _blockSize - 1, step);
        block.indices.push_back(RESTART_INDEX);

        block.pushRelativeIndex(_width, _blockSize - 1 - step, step);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, 2 * step);
        block.pushRelativeIndex(_width, _blockSize - 1, step);
        block.pushRelativeIndex(_width, _blockSize - 1, 2 * step);
        block.indices.push_back(RESTART_INDEX);

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
unsigned GeoMipMapping::loadBottomRightCorner(GeoMipMappingBlock& block, unsigned step, unsigned configuration)
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

        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, _blockSize - 1);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1);
        block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1 - 2 * step);
        block.indices.push_back(RESTART_INDEX);

        block.pushRelativeIndex(_width, _blockSize - 1 - step, _blockSize - 1 - 2 * step);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1 - 2 * step);
        block.indices.push_back(RESTART_INDEX);

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

        block.pushRelativeIndex(_width, _blockSize - 1 - step, _blockSize - 1);
        block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1 - 2 * step);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, _blockSize - 1 - 2 * step);
        block.indices.push_back(RESTART_INDEX);

        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, _blockSize - 1);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, _blockSize - 1);
        block.indices.push_back(RESTART_INDEX);

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

        block.pushRelativeIndex(_width, _blockSize - 1 - step, _blockSize - 1 - 2 * step);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1 - 2 * step);
        block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1 - step);
        block.indices.push_back(RESTART_INDEX);

        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, _blockSize - 1);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1);
        block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1 - step);
        block.indices.push_back(RESTART_INDEX);

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

        block.pushRelativeIndex(_width, _blockSize - 1 - step, _blockSize - 1 - 2 * step);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1 - 2 * step);
        block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1 - step);
        block.indices.push_back(RESTART_INDEX);

        // TODO
        block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1);
        block.pushRelativeIndex(_width, _blockSize - 1, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, _blockSize - 1);
        block.pushRelativeIndex(_width, _blockSize - 1 - step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, _blockSize - 1);
        block.pushRelativeIndex(_width, _blockSize - 1 - 2 * step, _blockSize - 1 - step);
        block.indices.push_back(RESTART_INDEX);

        count += 12;
    }

    return count;
}

unsigned GeoMipMapping::loadBottomLeftCorner(GeoMipMappingBlock& block, unsigned step, unsigned configuration)
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

        block.pushRelativeIndex(_width, step, _blockSize - 1 - 2 * step);
        block.pushRelativeIndex(_width, 0, _blockSize - 1 - 2 * step);
        block.pushRelativeIndex(_width, step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, 0, _blockSize - 1);
        block.pushRelativeIndex(_width, 2 * step, _blockSize - 1);
        block.indices.push_back(RESTART_INDEX);

        block.pushRelativeIndex(_width, step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, 2 * step, _blockSize - 1);
        block.pushRelativeIndex(_width, 2 * step, _blockSize - 1 - step);
        block.indices.push_back(RESTART_INDEX);

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

        block.pushRelativeIndex(_width, 2 * step, _blockSize - 1);
        block.pushRelativeIndex(_width, 2 * step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, step, _blockSize - 1);
        block.pushRelativeIndex(_width, step, _blockSize - 1 - step);
        block.indices.push_back(RESTART_INDEX);

        block.pushRelativeIndex(_width, step, _blockSize - 1 - 2 * step);
        block.pushRelativeIndex(_width, 0, _blockSize - 1 - 2 * step);
        block.pushRelativeIndex(_width, step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, 0, _blockSize - 1);
        block.pushRelativeIndex(_width, step, _blockSize - 1);
        block.indices.push_back(RESTART_INDEX);

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

        block.pushRelativeIndex(_width, 0, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, 0, _blockSize - 1);
        block.pushRelativeIndex(_width, step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, 2 * step, _blockSize - 1);
        block.pushRelativeIndex(_width, 2 * step, _blockSize - 1 - step);
        block.indices.push_back(RESTART_INDEX);

        block.pushRelativeIndex(_width, 0, _blockSize - 1 - 2 * step);
        block.pushRelativeIndex(_width, 0, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, step, _blockSize - 1 - 2 * step);
        block.pushRelativeIndex(_width, step, _blockSize - 1 - step);
        block.indices.push_back(RESTART_INDEX);

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

        block.pushRelativeIndex(_width, 2 * step, _blockSize - 1);
        block.pushRelativeIndex(_width, 2 * step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, step, _blockSize - 1);
        block.pushRelativeIndex(_width, step, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, 0, _blockSize - 1);
        block.pushRelativeIndex(_width, 0, _blockSize - 1 - step);
        block.indices.push_back(RESTART_INDEX);

        block.pushRelativeIndex(_width, 0, _blockSize - 1 - 2 * step);
        block.pushRelativeIndex(_width, 0, _blockSize - 1 - step);
        block.pushRelativeIndex(_width, step, _blockSize - 1 - 2 * step);
        block.pushRelativeIndex(_width, step, _blockSize - 1 - step);
        block.indices.push_back(RESTART_INDEX);

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
unsigned GeoMipMapping::loadTopBorder(GeoMipMappingBlock& block, unsigned step, unsigned configuration)
{
    unsigned count = 0;
    if (configuration & TOP_BORDER_BITMASK) {

        /* Load border with crack avoidance */
        for (int j = step * 2; j < (int)(_blockSize) - (int)step * 3; j += step * 2) {
            block.pushRelativeIndex(_width, j + 2 * step, step);
            block.pushRelativeIndex(_width, j + 2 * step, 0);
            block.pushRelativeIndex(_width, j + step, step);
            block.pushRelativeIndex(_width, j, 0);
            block.pushRelativeIndex(_width, j, step);
            block.indices.push_back(RESTART_INDEX);

            count += 6;
        }

    } else {
        /* Load border normally like any other block */
        for (int j = step * 2; j < (int)_blockSize - (int)step * 2; j += step) {
            block.pushRelativeIndex(_width, j, 0);
            block.pushRelativeIndex(_width, j, step);

            count += 2;
        }
    }
    block.indices.push_back(RESTART_INDEX);
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
unsigned GeoMipMapping::loadRightBorder(GeoMipMappingBlock& block, unsigned step, unsigned configuration)
{
    unsigned count = 0;

    if (configuration & RIGHT_BORDER_BITMASK) {
        // TODO
        for (int i = step * 2; i < (int)(_blockSize) - (int)step * 3; i += step * 2) {
            block.pushRelativeIndex(_width, _blockSize - 1 - step, i + 2 * step);
            block.pushRelativeIndex(_width, _blockSize - 1, i + 2 * step);
            block.pushRelativeIndex(_width, _blockSize - 1 - step, i + step);
            block.pushRelativeIndex(_width, _blockSize - 1, i);
            block.pushRelativeIndex(_width, _blockSize - 1 - step, i);
            block.indices.push_back(RESTART_INDEX);

            count += 6;
        }

    } else {
        /* Load border normally like any other block */
        for (int i = step * 2; i < (int)_blockSize - (int)step * 2; i += step) {
            block.pushRelativeIndex(_width, _blockSize - 1, i);
            block.pushRelativeIndex(_width, _blockSize - 1 - step, i);

            count += 2;
        }
    }

    block.indices.push_back(RESTART_INDEX);
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
unsigned GeoMipMapping::loadBottomBorder(GeoMipMappingBlock& block, unsigned step, unsigned configuration)
{
    unsigned count = 0;

    if (configuration & BOTTOM_BORDER_BITMASK) {
        for (int j = step * 2; j < (int)_blockSize - (int)step * 3; j += step * 2) {
            block.pushRelativeIndex(_width, j, _blockSize - step - 1);
            block.pushRelativeIndex(_width, j, _blockSize - 1);
            block.pushRelativeIndex(_width, j + step, _blockSize - step - 1);
            block.pushRelativeIndex(_width, j + 2 * step, _blockSize - 1);
            block.pushRelativeIndex(_width, j + 2 * step, _blockSize - step - 1);
            block.indices.push_back(RESTART_INDEX);

            count += 6;
        }
    } else {
        /* Load border normally like any other block */
        for (int j = step * 2; j < (int)_blockSize - (int)step * 2; j += step) {
            block.pushRelativeIndex(_width, j, _blockSize - 1 - step);
            block.pushRelativeIndex(_width, j, _blockSize - 1);

            count += 2;
        }
    }
    block.indices.push_back(RESTART_INDEX);
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
unsigned GeoMipMapping::loadLeftBorder(GeoMipMappingBlock& block, unsigned step, unsigned configuration)
{
    unsigned count = 0;

    if (configuration & LEFT_BORDER_BITMASK) {
        for (int i = step * 2; i < (int)(_blockSize) - (int)(step * 3); i += step * 2) {
            block.pushRelativeIndex(_width, step, i);
            block.pushRelativeIndex(_width, 0, i);
            block.pushRelativeIndex(_width, step, i + step);
            block.pushRelativeIndex(_width, 0, i + 2 * step);
            block.pushRelativeIndex(_width, step, i + 2 * step);
            block.indices.push_back(RESTART_INDEX);

            count += 6;
        }
    } else {
        /* Load border normally like any other block */
        for (int i = step * 2; i < (int)(_blockSize) - (int)(step * 2); i += step) {
            block.pushRelativeIndex(_width, step, i);
            block.pushRelativeIndex(_width, 0, i);

            count += 2;
        }
    }
    block.indices.push_back(RESTART_INDEX);
    count++;

    return count;
}

/*
 * TODO: implement below method for borders instead of having 4 methods for each side
 */
// unsigned GeoMipMapping::loadBorder(GeoMipMappingBlock& block, unsigned step, unsigned direction, unsigned configuration)
//{
//     unsigned count = 0;

//    if (configuration & LEFT_BORDER_BITMASK) {
//        for (int i = step * 2; i < (int)(_blockSize) - (int)(step * 3); i += step * 2) {
//            block.pushRelativeIndex(width, step, i);
//            block.pushRelativeIndex(width, 0, i);
//            block.pushRelativeIndex(width, step, i + step);
//            block.pushRelativeIndex(width, 0, i + 2 * step);
//            block.pushRelativeIndex(width, step, i + 2 * step);

//            count += 5;
//        }
//    } else {
//        /* Load border normally like any other block */
//        for (int i = step * 2; i < (int)(_blockSize) - (int)(step * 2); i += step) {
//            block.pushRelativeIndex(width, step, i);
//            block.pushRelativeIndex(width, 0, i);

//            count += 2;
//        }
//    }
//    block.indices.push_back(RESTART_INDEX);
//    count++;

//    return count;
//}

/**
 * @brief Unloads the vertex array object, vertex buffer object and for each
 *        block the element buffer objects
 */
void GeoMipMapping::unloadBuffers()
{
    std::cout << "Unloading buffers" << std::endl;
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);

    for (unsigned i = 0; i < _nBlocksZ; i++) {
        for (unsigned j = 0; j < _nBlocksX; j++) {
            unsigned currentEBO = getBlock(j, i)._ebo;
            glDeleteBuffers(1, &currentEBO);
        }
    }

    AtlodUtil::checkGlError("GeoMipMapping deletion failed");
}
