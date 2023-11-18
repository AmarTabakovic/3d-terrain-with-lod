#include "heightmap.h"

#include <iostream>

/**
 * @brief Heightmap::Heightmap
 * @param width
 * @param height
 */
Heightmap::Heightmap(unsigned int width, unsigned int height)
    : width(width)
    , height(height)
{
}

/**
 * @brief Heightmap::at
 * @param x
 * @param y
 * @return
 */
unsigned int Heightmap::at(unsigned int x, unsigned int z)
{
    /* z -> row, x -> column*/
    try {
        return data.at(z * width + x);
    } catch (std::out_of_range) {
        std::cout << "Failed fetching height at " << z << ", " << x << std::endl;
        std::exit(1);
    }
}

void Heightmap::push(unsigned int heightValue)
{
    data.push_back(heightValue);
}
