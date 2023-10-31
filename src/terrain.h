#ifndef TERRAIN_H
#define TERRAIN_H

#include "heightmap.h"
#include "shader.h"

#include <string>

class Terrain {
private:

public:
    Terrain(std::string& heightmapFile);
    void loadHeightmap(std::string& fileName);
    Heightmap heightmap;
    virtual void render() = 0;
    // Shader* shader = nullptr;
};

#endif // TERRAIN_H
