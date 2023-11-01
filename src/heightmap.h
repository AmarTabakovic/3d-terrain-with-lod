#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

#include <vector>

/**
 * @brief The Heightmap class
 */
class Heightmap {

public:
    std::vector<unsigned int> data;
    unsigned int width;
    unsigned int height;

    Heightmap(unsigned int width, unsigned int height);
    unsigned int at(unsigned int x, unsigned int z);
    void push(unsigned int heightValue);

    // int loadData(std::string& fileName);
};

#endif // HEIGHTMAP_H
