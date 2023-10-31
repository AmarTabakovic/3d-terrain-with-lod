#ifndef GEOMIPMAPPING_H
#define GEOMIPMAPPING_H

#include "terrain.h"

class GeoMipMapping : Terrain {
public:
    GeoMipMapping();
};

class Patch {
public:
    float distance;
    int lod;
};

#endif // GEOMIPMAPPING_H
