#include "geomipmapping.h"

#include <cmath>

/**
 * @brief GeoMipMapping::GeoMipMapping
 *
 * As of right now, the terrain's height data gets scaled down to the next
 * multiple of a patch size.
 *
 *
 * @param heightmapFileName
 * @param textureFileName
 */
GeoMipMapping::GeoMipMapping(std::string& heightmapFileName, std::string& textureFileName, unsigned int patchSize)
//: Terrain(heightmapFileName, textureFileName)
{
    shader = new Shader("../3d-terrain-with-lod/src/glsl/geomipmapping.vert", "../3d-terrain-with-lod/src/glsl/geomipmapping.frag");
    loadHeightmap(heightmapFileName);
    /* For now without texture */
    // loadTexture(textureFileName);

    unsigned int height = heightmap->height;
    unsigned int width = heightmap->width;

    /* Determine number of patches on x and z sides */
    nBlocksX = width / blockSize;
    nBlocksZ = height / blockSize;

    /* Calculate maximum LOD level */
    unsigned int divisor = blockSize - 1;
    while (divisor > 2) {
        divisor >>= 1;
        maxLod++;
    }

    for (unsigned int i = 0; i < nBlocksZ; i++) {
        for (unsigned int j = 0; j < nBlocksX; j++) {
            /* Initialize every block to have the minimum LOD. */
            blocks[i * nBlocksX + j] = { 0.0f, 0 };
        }
    }
}

/**
 * @brief GeoMipMapping::render
 */
void GeoMipMapping::render(Camera camera)
{
    /*
     * Check distance to camera of every patch and update LOD accordingly
     */
    for (unsigned int i = 0; i < nBlocksZ; i++) {
        for (unsigned int j = 0; j < nBlocksX; j++) {
            unsigned int x = j * blockSize + 0.5 * blockSize;
            unsigned int z = i * blockSize + 0.5 * blockSize;
            unsigned int y = heightmap->at(x, z);

            glm::vec3 blockCenter(x, y, z);
            glm::vec3 cameraPosition = camera.getPosition();

            /* TODO: Omit sqrtf for more performance? */
            float distance = std::sqrtf(
                std::pow(x - cameraPosition.x, 2.0f) + std::pow(y - cameraPosition.y, 2.0f) + std::pow(z - cameraPosition.z, 2.0f));

            unsigned int currBlock = i * nBlocksX + j;

            blocks[currBlock].distance = distance;

            /*
             * Naive LOD determination taken from "Focus on Terrain Programming"
             * TODO: Implement proper LOD determination
             */
            if (distance < 500) {
                blocks[currBlock].lod = maxLod;
            } else if (distance < 1000) {
                blocks[currBlock].lod = maxLod - 1;
            } else if (distance < 2500) {
                blocks[currBlock].lod = maxLod - 2;
            } else {
                blocks[currBlock].lod = maxLod - 3;
            }
        }
    }

    /* Update index buffer with appropriate patches */

    /* Render */
}

void GeoMipMapping::renderPatch()
{
}

/**
 * @brief GeoMipMapping::loadBuffers
 */
void GeoMipMapping::loadBuffers()
{
    unsigned int height = heightmap->height;
    unsigned int width = heightmap->width;
}

/**
 * @brief GeoMipMapping::unloadBuffers
 */
void GeoMipMapping::unloadBuffers()
{
}
