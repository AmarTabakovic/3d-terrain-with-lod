#ifndef GEOMIPMAPPINGQUADTREE_H
#define GEOMIPMAPPINGQUADTREE_H

#include "geomipmappingblock.h"

/**
 * @brief The GeoMipMappingQuadTree class
 *
 * Notes:
 * - In order to allow for quadtree-based frustum culling with non-square terrains,
 *   the size of the quadtree must be scaled up to thw next power of 2 accordingly.
 * - More specifically, the dimension of the quadtree must be n x n, where
 *   n is the next power of 2 greater than or equal to max(tWidth, tHeight),
 *   and tWidth and tHeight are the width and height of the terrain.
 *   The terrain is situated in the center of the quadtree.
 *
 *   The below figure shows a terrain of dimension n x 2n with a quadtree
 *   of dimension 2n x 2n enclosing it.
 *
 *   TODO Open questions: what about not centering it?
 *
 *   O- - - - - - – - - - - - -O
 *   |            |            |
 *   |            |            |
 *   .____________|____________.
 *   I            |            I
 *   I            |            I
 *   I- - - - - - O - - O - - -I
 *   I            |  |  |      I
 *   I            |- O -|      I
 *   .____________O__|__O______.
 *   |            |     |      |
 *   |            |     |      |
 *   |            |     |      |
 *   O- - - - - - - - - - - - -O
 *
 *   The symbols stand for:
 *   - 'O': Corners of the quadtree and its child nodes
 *   - '.': Corners of the terrain
 *   - '-' and '|': Borders of the quadtree and its child nodes
 *   - '_' and 'I': Borders of the terrain
 *
 */
class GeoMipMappingQuadTree {
public:
    //    struct QuadTreeNode {
    //        QuadTreeNode* _topLeftChild = nullptr;
    //        QuadTreeNode* _topRightChild = nullptr;
    //        QuadTreeNode* _bottomLeftChild = nullptr;
    //        QuadTreeNode* _bottomRightChild = nullptr;
    //        bool isLeafNode;
    //        GeoMipMappingBlock* block;
    //        unsigned width;
    //        glm::vec3 center;
    //        unsigned depth;
    //    };

    GeoMipMappingQuadTree(unsigned terrainWidth, unsigned terrainHeight /*, GeoMipMappingBlock* block = nullptr*/);
    ~GeoMipMappingQuadTree();

    void render(Camera& camera);

    // void build();
    // void buildRec(unsigned depth);

    void build(std::vector<GeoMipMappingBlock>& blocks);
    void buildRec(std::vector<GeoMipMappingBlock>& blocks, unsigned rowWidth, unsigned level, unsigned blockListPos);

    GeoMipMappingQuadTree* _topLeftChild = nullptr;
    GeoMipMappingQuadTree* _topRightChild = nullptr;
    GeoMipMappingQuadTree* _bottomLeftChild = nullptr;
    GeoMipMappingQuadTree* _bottomRightChild = nullptr;
    bool isLeafNode;
    GeoMipMappingBlock* block;
    unsigned width;
    glm::vec3 center;
    unsigned depth;
    unsigned _maxDepth;

    // GeoMipMappingQuadTree* _topLeftChild = nullptr;
    // GeoMipMappingQuadTree* _topRightChild = nullptr;
    // GeoMipMappingQuadTree* _bottomLeftChild = nullptr;
    // GeoMipMappingQuadTree* _bottomRightChild = nullptr;
    // GeoMipMappingBlock* _block;
    // bool _isLeafNode;
    // unsigned _width, _height;
};

#endif // GEOMIPMAPPINGQUADTREE_H
