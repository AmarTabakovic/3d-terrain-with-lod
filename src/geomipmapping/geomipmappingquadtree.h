#ifndef GEOMIPMAPPINGQUADTREE_H
#define GEOMIPMAPPINGQUADTREE_H

#include "geomipmappingblock.h"

class GeoMipMappingQuadTree {
public:
    GeoMipMappingQuadTree();
    GeoMipMappingQuadTree* topLeftChild = nullptr;
    GeoMipMappingQuadTree* topRight = nullptr;
    GeoMipMappingQuadTree* bottomLeft = nullptr;
    GeoMipMappingQuadTree* bottomRight = nullptr;
    GeoMipMappingBlock* block;
    unsigned int blockId;
    unsigned width;
    unsigned height;
};

#endif // GEOMIPMAPPINGQUADTREE_H
