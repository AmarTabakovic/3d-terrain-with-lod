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
unsigned char Heightmap::at(unsigned int x, unsigned int z)
{
    /* z -> row, x -> column*/
    try {
        return data.at(z * width + x);
    } catch (std::out_of_range) {
        std::cout << "Failed fetching height at " << z << ", " << x << "\n";
        std::exit(1);
    }
}

void Heightmap::insertAt(unsigned int x, unsigned int z, unsigned char heightValue)
{
    // data.insert((int)(z * height + x), heightValue);
}

void Heightmap::push(unsigned char heightValue)
{
    data.push_back(heightValue);
}

/*
int Heightmap::loadData(std::string& fileName)
{
    // Load from file name
    return 0;
}
*/
