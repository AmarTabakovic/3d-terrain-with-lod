#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

#include <vector>

/**
 * @brief The Heightmap class
 */
class Heightmap {

public:
    // Heightmap(unsigned width, unsigned height);
    Heightmap();
    void load(std::string& fileName);
    void loadImage(std::string& fileName);
    void loadAsciiGrid(std::string& fileName);
    void loadXyz(std::string& fileName);

    /* TODO: procedureal generation */
    void generate();

    /* Getters */
    unsigned width();
    unsigned height();

    unsigned at(unsigned x, unsigned z);
    void push(unsigned heightValue);
    void clear();

private:
    std::vector<unsigned> _data;
    unsigned _width;
    unsigned _height;
};

#endif // HEIGHTMAP_H
