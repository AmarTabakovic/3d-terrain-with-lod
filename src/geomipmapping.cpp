#include "geomipmapping.h"

#include <algorithm>
#include <cmath>

/**
 * @brief GeoMipMapping::GeoMipMapping
 *
 * As of right now, the terrain's height data gets scaled down to the next
 * multiple of a patch size.
 *
 * @param heightmapFileName
 * @param textureFileName
 */
GeoMipMapping::GeoMipMapping(std::string& heightmapFileName, std::string& textureFileName, unsigned int blockSize)
{
    std::cout << "Initialize GeoMipMapping" << std::endl;

    this->blockSize = blockSize;
    shader = new Shader("../3d-terrain-with-lod/src/glsl/geomipmapping.vert", "../3d-terrain-with-lod/src/glsl/geomipmapping.frag");
    loadHeightmap(heightmapFileName);
    loadTexture(textureFileName);

    /* Determine number of patches on x and z sides */
    nBlocksX = (unsigned int)std::floor((double)(heightmap->width) / (double)(blockSize - 1));
    nBlocksZ = (unsigned int)std::floor((double)(heightmap->height) / (double)(blockSize - 1));

    this->width = nBlocksX * (blockSize - 1) + 1;
    this->height = nBlocksZ * (blockSize - 1) + 1;

    /* Calculate maximum LOD level */
    maxLod = std::log2(blockSize - 1);

    std::cout << "blockSize: " << blockSize << std::endl
              << "nBlocksX: " << nBlocksX << std::endl
              << "nBlocksZ: " << nBlocksZ << std::endl
              << "maxLod: " << maxLod << std::endl;

    for (unsigned int i = 0; i < nBlocksZ; i++) {
        for (unsigned int j = 0; j < nBlocksX; j++) {
            /* Initialize every block to have the minimum LOD. TODO: find a better way to determine a starting index */
            unsigned int currentBlock = i * nBlocksX + j;
            unsigned int startIndex = i * width * (blockSize - 1) + (j * (blockSize - 1));
            blocks.push_back(GeoMipMappingBlock(currentBlock, startIndex));
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
    delete shader;
    delete heightmap;
    std::cout << "GeoMipMapping terrain destroyed" << std::endl;
}

/**
 * @brief GeoMipMapping::render
 */
void GeoMipMapping::render(Camera camera)
{
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RESTART);

    int signedHeight = (int)height; // nBlocksZ * (blockSize - 1) + 1;
    int signedWidth = (int)width; // nBlocksX * (blockSize - 1) + 1;

    glm::vec3 cameraPosition = camera.getPosition();

    /*
     * First pass: for each block:
     *  - Check and update distance to camera
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

            unsigned int currBlock = i * nBlocksX + j;

            blocks[currBlock].distance = distance;
            blocks[currBlock].currentLod = determineLod(distance);
            blocks[currBlock].currentBorderBitmap = 0; // TODO Actually update
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glBindVertexArray(terrainVAO);

    /*
     * Second pass: render each block
     */
    for (unsigned int i = 0; i < nBlocksZ; i++) {
        for (unsigned int j = 0; j < nBlocksX; j++) {
            unsigned int currBlock = i * nBlocksX + j;

            unsigned int currentLod = blocks[currBlock].currentLod;
            unsigned int currentBorderBitmap = blocks[currBlock].currentBorderBitmap;

            unsigned int currentIndex = currentLod * 16 + currentBorderBitmap;

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, blocks[currBlock].ebo);
            glDrawElements(GL_TRIANGLE_STRIP, blocks[currBlock].subBufferSizes[currentIndex], GL_UNSIGNED_INT, (void*)(blocks[currBlock].subBufferStarts[currentIndex] * sizeof(unsigned int)));
        }
    }

    GLenum error = glGetError();
    if (error != 0) {
        std::cout << "Error after new index buffer loading: " << error << std::endl;
    }
}

unsigned int GeoMipMapping::determineLod(float distance)
{
    /*
     * Naive LOD determination taken from "Focus on Terrain Programming"
     * TODO: Implement proper LOD determination
     */
    if (distance < 100) {
        return maxLod;
    } else if (distance < 250) {
        return maxLod - 1;
    } else if (distance < 500) {
        return maxLod - 2;
    } else if (distance < 750) {
        return maxLod - 3;
    } else if (distance < 1000) {
        return maxLod - 4;
    } else if (distance < 1250) {
        return maxLod - 5;
    } else
        return maxLod - 6;
    /* } else if (distance < 1500) {
         return maxLod - 6;
     } else if (distance < 1750) {
         return maxLod - 7;
     } else
         return maxLod - 8;*/
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

    loadIndexBuffers();

    GLenum error = glGetError();
    if (error != 0) {
        std::cout << "Error " << std::endl;
    }
}

void GeoMipMapping::loadGeoMipMapsForBlock(GeoMipMappingBlock& block)
{
    unsigned int totalCount = 0;
    for (unsigned int i = 0; i <= maxLod; i++) {
        GeoMipMap currentGeoMipMap = GeoMipMap(i);
        totalCount = loadBorderConfigurationsForGeoMipMap(block, i, block.startIndex, totalCount);
        block.geoMipMaps.push_back(currentGeoMipMap);
    }

    glGenBuffers(1, &block.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, block.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, block.indexBuffer.size() * sizeof(unsigned int), &block.indexBuffer[0], GL_STATIC_DRAW);
}

unsigned int GeoMipMapping::createIndicesForConfig(GeoMipMappingBlock& block, unsigned int configBitmap, unsigned int lod, unsigned int startIndex)
{
    unsigned int step = std::pow(2, maxLod - lod);

    unsigned int count = 0;

    /* For now only do normal cracked blocks, just for testing */
    for (unsigned int i = 0; i < blockSize - 1; i += step) {
        for (unsigned int j = 0; j < blockSize; j += step) {
            block.indexBuffer.push_back(i * width + startIndex + j);
            block.indexBuffer.push_back((i + 1 * step) * width + startIndex + j);
            count += 2;
        }
        block.indexBuffer.push_back(RESTART);
        count++;
    }

    block.subBufferSizes.push_back(count);

    return count;
}

/**
 * @brief GeoMipMapping::loadBorderConfigurationsForGeoMipMap
 *
 * TODO: There is probably a better way to solve this than with accumulatedCounts...
 *
 * @param block
 * @param lod
 * @param startIndex
 * @param accumulatedCounts
 * @return
 */
unsigned int GeoMipMapping::loadBorderConfigurationsForGeoMipMap(GeoMipMappingBlock& block, unsigned int lod, unsigned int startIndex, unsigned int accumulatedCounts)
{
    /* 2^4 = 16 possible combinations for border configurations */
    unsigned int totalCount = 0;
    for (unsigned int i = 0; i < 16; i++) {
        unsigned int count = createIndicesForConfig(block, i, lod, startIndex);
        totalCount += count;
        block.subBufferStarts.push_back(accumulatedCounts + totalCount - count);
    }
    return totalCount;
}

GeoMipMap::GeoMipMap(unsigned int lod)
{
    this->lod = lod;
    // this->indexBuffers = {};
}

void GeoMipMapping::loadIndexBuffers()
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
            /*for (unsigned int k = 0; k <= maxLod; k++) {
                for (unsigned int l = 0; l < 16; l++) {
                    unsigned int currBlock = i * nBlocksX + j;
                    unsigned int currentEBO = blocks[currBlock].geoMipMaps[k].indexBuffers[l];
                    glDeleteBuffers(1, &currentEBO);
                }
            }*/
            unsigned int currBlock = i * nBlocksX + j;
            unsigned int currentEBO = blocks[currBlock].ebo;
            glDeleteBuffers(1, &currentEBO);
        }
    }
}
