/* UNUSED */
// #include "geomipmappingquadtree.h"

// GeoMipMappingQuadTree::GeoMipMappingQuadTree(unsigned maxDepth /*unsigned terrainWidth, unsigned terrainHeight*/ /*, float nodeWidth, GeoMipMappingBlock* block*/)
//{
//     // unsigned powerOf2Width = std::ceil(std::log2(terrainWidth));
//     // unsigned powerOf2Height = std::ceil(std::log2(terrainHeight));

//    /* Scale up quadtree to next power of 2 */
//    // unsigned dimension = std::max(powerOf2Width, powerOf2Height);

//    //_block = block;
//    //_width = 1 << dimension;
//    //_height = 1 << dimension;
//    _maxDepth = maxDepth; // dimension;

//    /* TODO: Node width is either blockWidth (leaf node) or a multiple of that*/
//    //_nodeWidth = nodeWidth;
//    /*TODO center field*/
//    //_center = glm::vec3(0, 0, 0);
//}

///**
// * @brief GeoMipMappingQuadTree::GeoMipMappingQuadTree
// *
// * TODO: Free children
// */
// GeoMipMappingQuadTree::~GeoMipMappingQuadTree()
//{
//}

//// bool GeoMipMappingQuadTree::isLeafNode()
////{
////     return _isLeafNode;
//// }

///**
// * @brief GeoMipMappingQuadTree::build
// *
// * Steps:
// * - From top-left to bottom-right, insert leaf nodes with block refs,
// *   while keeping track of the position in the block list
// */
// void GeoMipMappingQuadTree::build(std::vector<GeoMipMappingBlock>& blocks, unsigned nBlocksX, unsigned nBlocksZ)
//{
//    /*_topLeftChild = GeoMipMappingQuadTree()
//    _topRightChild
//    _bottomLeftChild
//    _bottomRightChild*/
//}

///**
// * @brief GeoMipMappingQuadTree::buildRec
// *
// * rowWidth is the width of a single row in the block list (i.e. nBlocksX)
// *
// * @param blocks
// * @param rowWidth
// * @param level
// * @param blockListPos
// */
// void GeoMipMappingQuadTree::buildRec(std::vector<GeoMipMappingBlock>& blocks, unsigned rowWidth, unsigned level, unsigned blockX, unsigned blockY)
//{
//    if (level == _maxDepth) {
//    }

//    left
//}

///**
// * @brief GeoMipMappingQuadTree::visit
// *
// * Idea:
// * - Start at root node
// * - Check for each of children whether their AABB interesects with frustum
// * - If true, visit them, else, ignore
// * - Continue this recursively until the current node is a leaf node
// * - If leaf node's AABB intersects frustum, render
// */
// void GeoMipMappingQuadTree::render(Camera& camera)
//{
//    if (isLeafNode) {
//        if (true) { /* TODO: Camera intersects block AABB */
//            /* OpenGL rendering stuff */
//        }
//    } else if (true) { /* TODO: Camera intersects quadtree AABB */
//        _topLeftChild->render(camera);
//    }
//}
