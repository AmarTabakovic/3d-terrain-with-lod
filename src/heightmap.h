#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

#include <vector>

class Heightmap {

public:
    Heightmap();
    void load(const std::string& fileName, bool loadTextureHeightmap);
    void loadImage(const std::string& fileName, bool loadTextureHeightmap);

    void loadTexture(unsigned short* data);
    void unloadTexture();
    unsigned heightmapTextureId();

    /* Getters */
    unsigned width();
    unsigned height();
    std::vector<unsigned short> data();

    unsigned at(unsigned x, unsigned z);
    void clear();

    /* Minimum and maximum heightmap y-values */
    unsigned short min, max;

private:
    std::vector<unsigned short> _data;
    unsigned _width;
    unsigned _height;
    unsigned _heightmapTextureId;
};

#endif // HEIGHTMAP_H
