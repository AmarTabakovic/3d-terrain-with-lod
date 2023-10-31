#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

#include <vector>

/**
 * @brief The Heightmap class
 */
class Heightmap {

public:
    std::vector<unsigned char> data;
    unsigned int width;
    unsigned int height;
    Heightmap(unsigned int width, unsigned int height);

    unsigned char at(unsigned int x, unsigned int z);
    void insertAt(unsigned int x, unsigned int z, unsigned char heightValue);
    void push(unsigned char heightValue);

    // int loadData(std::string& fileName);
};

#endif // HEIGHTMAP_H
