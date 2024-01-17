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
    void load(const std::string& fileName);
    void loadImage(const std::string& fileName);
    // void loadAsciiGrid(std::string& fileName);
    // void loadXyz(std::string& fileName);

    void generateGlTexture();
    void destroyGlTexture();
    unsigned heightmapTextureId();

    /* Getters */
    unsigned width();
    unsigned height();
    std::vector<unsigned short> data();

    unsigned at(unsigned x, unsigned z);
    void push(unsigned heightValue);
    void clear();

    /* Minimum and maximum heightmap y-values */
    unsigned short min, max;

    unsigned short* tempData;

private:
    std::vector<unsigned short> _data;
    unsigned _width;
    unsigned _height;
    unsigned _heightmapTextureId;
};

#endif // HEIGHTMAP_H
