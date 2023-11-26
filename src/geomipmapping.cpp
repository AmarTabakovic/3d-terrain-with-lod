#include "geomipmapping.h"
#include "atlodutil.h"

#include <algorithm>
#include <cmath>

/**
 * @brief GeoMipMapping::GeoMipMapping
 *
 * As of right now, the terrain's height data gets scaled down to the next
 * multiple of a patch size.
 *
 * TODO: Appropriate constructor initialization...
 *
 * @param heightmapFileName
 * @param textureFileName
 */
GeoMipMapping::GeoMipMapping(std::string& heightmapFileName, std::string& textureFileName, unsigned int blockSize)
{
    std::cout << "Initialize GeoMipMapping" << std::endl;

    this->blockSize = blockSize;
    this->shader = new Shader("../3d-terrain-with-lod/src/glsl/geomipmapping.vert", "../3d-terrain-with-lod/src/glsl/geomipmapping.frag");
    loadHeightmap(heightmapFileName);
    loadTexture(textureFileName);

    /* Determine number of patches on x and z sides
     * TODO: This casting and flooring is probably not required...
     */
    this->nBlocksX = std::floor((heightmap->width - 1) / (blockSize - 1));
    this->nBlocksZ = std::floor((heightmap->height - 1) / (blockSize - 1));

    this->width = nBlocksX * (blockSize - 1) + 1;
    this->height = nBlocksZ * (blockSize - 1) + 1;

    /* Calculate maximum LOD level */
    this->maxLod = std::log2(blockSize - 1);

    std::cout << "blockSize: " << blockSize << std::endl
              << "nBlocksX: " << nBlocksX << std::endl
              << "nBlocksZ: " << nBlocksZ << std::endl
              << "maxLod: " << maxLod << std::endl;

    for (unsigned int i = 0; i < nBlocksZ; i++) {
        for (unsigned int j = 0; j < nBlocksX; j++) {
            unsigned int currentBlock = i * nBlocksX + j;
            unsigned int startIndex = i * width * (blockSize - 1) + (j * (blockSize - 1));
            this->blocks.push_back(GeoMipMappingBlock(currentBlock, startIndex));
        }
    }
}

GeoMipMappingBlock::GeoMipMappingBlock(unsigned int blockId, unsigned int startIndex)
{
    this->distance = 0;
    this->blockId = blockId;
    this->startIndex = startIndex;
    this->currentLod = 0;
    this->currentBorderBitmap = 0;
    this->geoMipMaps = {};
}

/**
 * @brief GeoMipMapping::~GeoMipMapping
 */
GeoMipMapping::~GeoMipMapping()
{
    delete this->shader;
    delete this->heightmap;
    std::cout << "GeoMipMapping terrain destroyed" << std::endl;
}

/**
 * @brief GeoMipMapping::calculateBorderBitmap
 * @param currentBlockId
 * @param x
 * @param y
 * @return
 */
unsigned int GeoMipMapping::calculateBorderBitmap(unsigned int currentBlockId, unsigned int x, unsigned int z)
{
    unsigned int currentLod = blocks[currentBlockId].currentLod;

    /* TODO: problems with unsigned... */
    GeoMipMappingBlock& leftBlock = getBlock(std::max((int)(x)-1, 0), z);
    GeoMipMappingBlock& rightBlock = getBlock(std::min((int)(x) + 1, (int)nBlocksX - 1), z);
    GeoMipMappingBlock& topBlock = getBlock(x, std::max((int)(z)-1, 0));
    GeoMipMappingBlock& bottomBlock = getBlock(x, std::min((int)(z) + 1, (int)nBlocksZ - 1));

    unsigned int leftLower = currentLod > leftBlock.currentLod ? 1 : 0;
    unsigned int rightLower = currentLod > rightBlock.currentLod ? 1 : 0;
    unsigned int topLower = currentLod > topBlock.currentLod ? 1 : 0;
    unsigned int bottomLower = currentLod > bottomBlock.currentLod ? 1 : 0;

    unsigned int bitmask = (leftLower << 3) | (rightLower << 2) | (topLower << 1) | bottomLower;

    // std::cout << bitmask << std::endl;
    return bitmask;
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
GeoMipMappingBlock& GeoMipMapping::getBlock(unsigned int x, unsigned int z)
{
    return blocks[z * nBlocksX + x];
}

/**
 * @brief GeoMipMapping::render
 */
void GeoMipMapping::render(Camera camera)
{
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RESTART_INDEX);

    int signedHeight = (int)height;
    int signedWidth = (int)width;

    glm::vec3 cameraPosition = camera.getPosition();

    /*
     * First pass: for each block:
     *  - Check and update the distance to camera
     *  - Update the LOD level
     *  - Update the neighborhood border bitmap
     */
    for (unsigned int i = 0; i < nBlocksZ; i++) {
        for (unsigned int j = 0; j < nBlocksX; j++) {
            float x = (j * (blockSize - 1) + 0.5 * (blockSize - 1));
            float z = (i * (blockSize - 1) + 0.5 * (blockSize - 1));
            float y = heightmap->at(x, z);
            glm::vec3 blockCenter(((-signedWidth * xzScale) / 2) + x * xzScale, y * yScale, ((-signedHeight * xzScale) / 2) + z * xzScale);

            float distance = glm::distance(cameraPosition, blockCenter);

            // unsigned int currBlock = i * nBlocksX + j;
            GeoMipMappingBlock& block = getBlock(j, i);

            block.distance = distance;
            block.currentLod = determineLod(distance);
            block.currentBorderBitmap = calculateBorderBitmap(block.blockId, j, i);
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glBindVertexArray(terrainVAO);

    /*
     * Second pass: render each block
     *
     * TODO: Update and render everything in one pass instead?
     */
    for (unsigned int i = 0; i < nBlocksZ; i++) {
        for (unsigned int j = 0; j < nBlocksX; j++) {
            GeoMipMappingBlock& block = getBlock(j, i);

            unsigned int currentIndex = block.currentLod;

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, block.ebo);

            /* First render the center subblocks */
            glDrawElements(GL_TRIANGLE_STRIP,
                block.centerSizes[currentIndex],
                GL_UNSIGNED_INT,
                (void*)(block.centerStarts[currentIndex] * sizeof(unsigned int)));

            /* Then render the border subblocks */
            currentIndex = currentIndex * 16 + block.currentBorderBitmap;

            glDrawElements(GL_TRIANGLE_STRIP,
                block.borderSizes[currentIndex],
                GL_UNSIGNED_INT,
                (void*)(block.borderStarts[currentIndex] * sizeof(unsigned int)));
        }
    }

    AtlodUtil::checkGlError("GeoMipMapping render failed");
}

/**
 * @brief GeoMipMapping::determineLod
 * @param distance
 * @return
 */
unsigned int GeoMipMapping::determineLod(float distance)
{
    /*
     * Naive LOD determination taken from "Focus on Terrain Programming"
     * TODO: Implement proper LOD determination
     */
    float baseDist = 100.0f;
    if (distance < 1 * baseDist) {
        return maxLod;
    } else if (distance < 2 * baseDist) {
        return maxLod - 1;
    } else if (distance < 3 * baseDist) {
        return maxLod - 2;
    } else if (distance < 4 * baseDist) {
        return maxLod - 3;
    } else if (distance < 5 * baseDist) {
        return maxLod - 4;
    } else if (distance < 6 * baseDist) {
        return maxLod - 5;
    } else
        return maxLod - 6;
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
    /* Need to be int, not unsigned */
    int signedHeight = (int)height;
    int signedWidth = (int)width;

    /* Set up vertex buffer */
    for (unsigned int i = 0; i < height; i++) {
        for (unsigned int j = 0; j < width; j++) {

            float y = heightmap->at(j, i);

            /* Load vertices around center point */
            float x = (-signedWidth / 2.0f + signedWidth * j / (float)signedWidth);
            float z = (-signedHeight / 2.0f + signedHeight * i / (float)signedHeight);

            vertices.push_back(x); /* vertex x */
            vertices.push_back(y); /* vertex y */
            vertices.push_back(z); /* vertex z */
            vertices.push_back((float)j / (float)heightmap->width); /* texture x */
            vertices.push_back((float)i / (float)heightmap->height); /* texture y */
        }
    }

    glGenVertexArrays(1, &terrainVAO);
    glBindVertexArray(terrainVAO);

    glGenBuffers(1, &terrainVBO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    /* Position attribute */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    /* Texture attribute */
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    loadIndices();

    AtlodUtil::checkGlError("GeoMipMapping load failed");
}

/**
 * @brief GeoMipMapping::loadGeoMipMapsForBlock
 * @param block
 */
void GeoMipMapping::loadGeoMipMapsForBlock(GeoMipMappingBlock& block)
{
    unsigned int totalBorderCount = 0, totalCenterCount = 0;
    unsigned int totalCount = 0;

    for (unsigned int i = 0; i <= maxLod; i++) {
        GeoMipMap currentGeoMipMap = GeoMipMap(i);

        /* Load border subblocks */
        unsigned int borderCount = loadBorderAreaForLod(block, block.startIndex, i, totalCount);
        totalBorderCount += borderCount;
        totalCount += borderCount;

        /* Load center subblocks */
        unsigned int centerCount = loadCenterAreaForLod(block, block.startIndex, i);
        totalCenterCount += centerCount;
        totalCount += centerCount;

        block.centerStarts.push_back(totalCount - centerCount);

        block.geoMipMaps.push_back(currentGeoMipMap);
    }

    glGenBuffers(1, &block.ebo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, block.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, block.indices.size() * sizeof(unsigned int), &block.indices[0], GL_STATIC_DRAW);

    /* TODO: Clear temporary index vector for less memory consumption?
     *       The indices are in the EBO anyway...
     */
    block.indices.clear();
}

/**
 * @brief GeoMipMapping::loadSingleBorderSide
 * @param block
 * @param startIndex
 * @param lod
 * @param currentSide
 * @param leftSideHigher
 * @param rightSideHigher
 * @return
 */
unsigned int GeoMipMapping::loadSingleBorderSide(GeoMipMappingBlock& block, unsigned int startIndex, unsigned int lod, unsigned int currentSide, bool leftSideHigher, bool rightSideHigher)
{
    return 0;
}

/**
 * @brief GeoMipMapping::loadBorderAreaForConfiguration
 * @param block
 * @param startIndex
 * @param lod
 * @param configuration
 * @return
 */
unsigned int GeoMipMapping::loadBorderAreaForConfiguration(GeoMipMappingBlock& block, unsigned int startIndex, unsigned int lod, unsigned int configuration)
{

    /*
     * The idea is to traverse the border subblocks clockwise starting from the top left corner.
     *
     * TODO:
     * - Vertex omission cases
     * - Corners (for now no special corner cases were implemented, mainly for testing purposes)
     * - Refactor the below left, right, top, bottom cases into single methods
     * - Clean up the indexing (e.g. zero-multiplications, etc:)
     * - Determine way to access single indices inside single blocks using x, y.
     */

    unsigned int step = std::pow(2, maxLod - lod);
    unsigned int count = 0;
    // bool onCorner = true;

    /* ========================== Top left corner ========================= */
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

        block.indices.push_back((0 + 2 * step) * width + startIndex + 0 + 1 * step);
        block.indices.push_back((0 + 2 * step) * width + startIndex + 0 + 0 * step);
        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 1 * step);
        block.indices.push_back((0 + 0 * step) * width + startIndex + 0 + 0 * step);
        block.indices.push_back((0 + 0 * step) * width + startIndex + 0 + 2 * step);
        block.indices.push_back(RESTART_INDEX);
        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 1 * step);
        block.indices.push_back((0 + 0 * step) * width + startIndex + 0 + 2 * step);
        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 2 * step);
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

        block.indices.push_back((0 + 2 * step) * width + startIndex + 0 + 1 * step);
        block.indices.push_back((0 + 2 * step) * width + startIndex + 0 + 0 * step);
        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 1 * step);
        block.indices.push_back((0 + 0 * step) * width + startIndex + 0 + 0 * step);
        block.indices.push_back((0 + 0 * step) * width + startIndex + 0 + 1 * step);
        block.indices.push_back(RESTART_INDEX);
        block.indices.push_back((0 + 0 * step) * width + startIndex + 0 + 1 * step);
        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 1 * step);
        block.indices.push_back((0 + 0 * step) * width + startIndex + 0 + 2 * step);
        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 2 * step);
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
        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 0 * step);
        block.indices.push_back((0 + 2 * step) * width + startIndex + 0 + 0 * step);
        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 1 * step);
        block.indices.push_back((0 + 2 * step) * width + startIndex + 0 + 1 * step);
        block.indices.push_back(RESTART_INDEX);

        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 0 * step);
        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 1 * step);
        block.indices.push_back((0 + 0 * step) * width + startIndex + 0 + 0 * step);
        block.indices.push_back((0 + 0 * step) * width + startIndex + 0 + 2 * step);
        block.indices.push_back(RESTART_INDEX);

        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 1 * step);
        block.indices.push_back((0 + 0 * step) * width + startIndex + 0 + 2 * step);
        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 2 * step);
        block.indices.push_back(RESTART_INDEX);

        count += 14;

    } else if (!(configuration & TOP_BORDER_BITMASK) && !(configuration & LEFT_BORDER_BITMASK)) { /* bitmask is 0_0_*/
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
        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 0 * step);
        block.indices.push_back((0 + 2 * step) * width + startIndex + 0 + 0 * step);
        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 1 * step);
        block.indices.push_back((0 + 2 * step) * width + startIndex + 0 + 1 * step);
        block.indices.push_back(RESTART_INDEX);

        block.indices.push_back((0 + 0 * step) * width + startIndex + 0 + 0 * step);
        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 0 * step);
        block.indices.push_back((0 + 0 * step) * width + startIndex + 0 + 1 * step);
        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 1 * step);
        block.indices.push_back((0 + 0 * step) * width + startIndex + 0 + 2 * step);
        block.indices.push_back((0 + 1 * step) * width + startIndex + 0 + 2 * step);
        block.indices.push_back(RESTART_INDEX);

        count += 12;
    }

    /* ============================= Top side ============================= */
    //    if (configuration & TOP_BORDER_BITMASK) {

    //        /* Load border with crack avoidance */
    //        for (unsigned int j = 0; j < blockSize - step; j += step * 2) {
    //            block.indices.push_back((0 + 1 * step) * width + startIndex + j);
    //            block.indices.push_back(0 * width + startIndex + j);
    //            block.indices.push_back((0 + 1 * step) * width + startIndex + j + 1 * step);
    //            block.indices.push_back(0 * width + startIndex + j + 2 * step);
    //            block.indices.push_back((0 + 1 * step) * width + startIndex + j + 2 * step);
    //            block.indices.push_back(RESTART_INDEX);
    //            count += 6;
    //        }
    //        block.indices.push_back(RESTART_INDEX);
    //        count++;

    //    } else {
    //        /* Load border normally like any other block */
    //        for (unsigned int j = step; j < blockSize - step; j += step) {
    //            block.indices.push_back(0 * width + startIndex + j);
    //            block.indices.push_back((0 + 1 * step) * width + startIndex + j);
    //            count += 2;
    //        }
    //        block.indices.push_back(RESTART_INDEX);
    //        count++;
    //    }

    if (configuration & TOP_BORDER_BITMASK) {

        /* Load border with crack avoidance */
        for (int j = step * 2; j < ((int)(blockSize) - ((int)step * 3)); j += step * 2) {
            std::cout << j << std::endl;
            block.indices.push_back((0 + 1 * step) * width + startIndex + j);
            block.indices.push_back(0 * width + startIndex + j);
            block.indices.push_back((0 + 1 * step) * width + startIndex + j + 1 * step);
            block.indices.push_back(0 * width + startIndex + j + 2 * step);
            block.indices.push_back((0 + 1 * step) * width + startIndex + j + 2 * step);
            block.indices.push_back(RESTART_INDEX);
            count += 6;
        }
        block.indices.push_back(RESTART_INDEX);
        count++;

    } else {
        /* Load border normally like any other block */
        for (int j = step * 2; j < ((int)(blockSize) - ((int)step * 2)); j += step) {
            block.indices.push_back(0 * width + startIndex + j);
            block.indices.push_back((0 + 1 * step) * width + startIndex + j);
            count += 2;
        }
        block.indices.push_back(RESTART_INDEX);
        count++;
    }

    /* ============================ Right side ============================ */
    if (configuration & RIGHT_BORDER_BITMASK) {
        for (unsigned int i = 0; i < blockSize - step; i += step * 2) {
            block.indices.push_back(i * width + startIndex + blockSize - 1 * step - 1);
            block.indices.push_back(i * width + startIndex + blockSize - 0 * step - 1);
            block.indices.push_back((i + 1 * step) * width + startIndex + blockSize - 1 * step - 1);
            block.indices.push_back((i + 2 * step) * width + startIndex + blockSize - 0 * step - 1);
            block.indices.push_back((i + 2 * step) * width + startIndex + blockSize - 1 * step - 1);
            block.indices.push_back(RESTART_INDEX);
            count += 6;
        }
        block.indices.push_back(RESTART_INDEX);
        count++;
    } else {
        /* Load border normally like any other block*/
        for (unsigned int i = step; i < blockSize - step; i += step) {
            block.indices.push_back(i * width + startIndex + blockSize - 1 * step - 1);
            block.indices.push_back(i * width + startIndex + blockSize - 0 * step - 1);
            count += 2;
        }
        block.indices.push_back(RESTART_INDEX);
        count++;
    }

    /* ============================ Bottom side =========================== */
    if (configuration & BOTTOM_BORDER_BITMASK) {
        for (unsigned int j = 0; j < blockSize - step; j += step * 2) {

            block.indices.push_back((blockSize - step - 1) * width + startIndex + j);
            block.indices.push_back(((blockSize - step - 1) + 1 * step) * width + startIndex + j);
            block.indices.push_back((blockSize - step - 1) * width + startIndex + j + 1 * step);
            block.indices.push_back(((blockSize - step - 1) + 1 * step) * width + startIndex + j + 2 * step);
            block.indices.push_back(((blockSize - step - 1)) * width + startIndex + j + 2 * step);
            block.indices.push_back(RESTART_INDEX);
            count += 6;
        }
        block.indices.push_back(RESTART_INDEX);
        count++;
    } else {
        /* Load border normally like any other block */
        for (unsigned int j = step; j < blockSize - step; j += step) {
            block.indices.push_back((blockSize - step - 1) * width + startIndex + j);
            block.indices.push_back(((blockSize - step - 1) + 1 * step) * width + startIndex + j);
            count += 2;
        }
        block.indices.push_back(RESTART_INDEX);
        count++;
    }

    /* ============================= Left side ============================ */
    if (configuration & LEFT_BORDER_BITMASK) {
        for (unsigned int i = 0; i < blockSize - step; i += step * 2) {
            block.indices.push_back(i * width + startIndex + 1 * step);
            block.indices.push_back(i * width + startIndex);
            block.indices.push_back((i + 1 * step) * width + startIndex + 1 * step);
            block.indices.push_back((i + 2 * step) * width + startIndex);
            block.indices.push_back((i + 2 * step) * width + startIndex + 1 * step);
            block.indices.push_back(RESTART_INDEX);
            count += 6;
        }
        block.indices.push_back(RESTART_INDEX);
        count++;
    } else {
        /* Load border normally like any other block */
        for (unsigned int i = step; i < blockSize - step; i += step) {
            block.indices.push_back(i * width + startIndex + 0 * step);
            block.indices.push_back(i * width + startIndex + 1 * step);
            count += 2;
        }
        block.indices.push_back(RESTART_INDEX);
        count++;
    }

    block.borderSizes.push_back(count);

    return count;
}

/**
 * @brief GeoMipMapping::loadBorderAreaForLod
 * @param block
 * @param startIndex
 * @param lod
 * @return
 */
unsigned int GeoMipMapping::loadBorderAreaForLod(GeoMipMappingBlock& block, unsigned int startIndex, unsigned int lod, unsigned int accumulatedCount)
{
    unsigned int totalCount = 0;

    /* 2^4 = 16 possible combinations */
    /* TODO: Load LOD 0 manually? */
    for (int i = 0; i < 16; i++) {
        unsigned int count = loadBorderAreaForConfiguration(block, startIndex, lod, i);
        totalCount += count;
        accumulatedCount += count;
        block.borderStarts.push_back(accumulatedCount - count);
    }

    return totalCount;
}

/**
 * @brief GeoMipMapping::loadCenterAreaForLod
 * @param block
 * @param startIndex
 * @param lod
 * @return
 */
unsigned int GeoMipMapping::loadCenterAreaForLod(GeoMipMappingBlock& block, unsigned int startIndex, unsigned int lod)
{
    unsigned int step = std::pow(2, maxLod - lod);

    unsigned int count = 0;

    for (unsigned int i = step; i < blockSize - step - 1; i += step) {
        for (unsigned int j = step; j < blockSize - step; j += step) {
            block.indices.push_back(i * width + startIndex + j);
            block.indices.push_back((i + 1 * step) * width + startIndex + j);
            count += 2;
        }
        block.indices.push_back(RESTART_INDEX);
        count++;
    }

    block.centerSizes.push_back(count);

    return count;
}

/**
 * @brief GeoMipMap::GeoMipMap
 * @param lod
 */
GeoMipMap::GeoMipMap(unsigned int lod)
{
    this->lod = lod;
}

/**
 * @brief GeoMipMapping::loadIndices
 */
void GeoMipMapping::loadIndices()
{
    for (unsigned int i = 0; i < nBlocksZ; i++) {
        for (unsigned int j = 0; j < nBlocksX; j++) {
            unsigned int currentBlock = i * nBlocksX + j;
            loadGeoMipMapsForBlock(blocks[currentBlock]);
        }
    }
}

/**
 * @brief GeoMipMapping::unloadBuffers
 */
void GeoMipMapping::unloadBuffers()
{
    std::cout << "Unloading buffers" << std::endl;
    glDeleteVertexArrays(1, &terrainVAO);
    glDeleteBuffers(1, &terrainVBO);

    for (unsigned int i = 0; i < nBlocksZ; i++) {
        for (unsigned int j = 0; j < nBlocksX; j++) {
            unsigned int currBlock = i * nBlocksX + j;
            unsigned int currentEBO = blocks[currBlock].ebo;
            glDeleteBuffers(1, &currentEBO);
        }
    }

    AtlodUtil::checkGlError("GeoMipMapping deletion failed");
}
