#ifndef BVH_UTILS_H
#define BVH_UTILS_H

#include <memory>
#include <array>

#include <glm/glm.hpp>

#include "model.h"


struct alignas(16) BVHNode {
    glm::vec3 minVertPos;
    int firstFaceIndex;
    glm::vec3 maxVertPos;
    int lastFaceIndex;
    bool isLeaf;
    int missIndex;

    BVHNode(glm::vec3 minVertPos, glm::vec3 maxVertPos, bool isLeaf, int missIndex, int firstFaceIndex, int lastFaceIndex) 
        : minVertPos(minVertPos), maxVertPos(maxVertPos), isLeaf(isLeaf), 
        missIndex(missIndex), firstFaceIndex(firstFaceIndex), lastFaceIndex(lastFaceIndex) {

    }
};

struct BVHTree {
    BVHNode bvhNode;
    std::unique_ptr<BVHTree> leftChild;
    std::unique_ptr<BVHTree> rightChild;
    bool isRoot;

    BVHTree(BVHNode bvhNode) : bvhNode(bvhNode) {
        isRoot = false;
    }
};

class BVHUtils {
public:
    std::unique_ptr<BVHTree> subdivideModel(glm::vec3 minPoint, glm::vec3 maxPoint,
                           std::vector<std::array<int,3>>& modelFaces,
                           std::vector<glm::vec3>& centroids,
                           std::vector<glm::vec3>& modelVertices,
                           std::vector<glm::ivec4>& indices, 
                           int numberOfFacesInLeaves);

    void addBVHTreeToBVHNodes(BVHTree& bvh, int missIndex, bool isRight, std::vector<BVHNode>& bvhNodes);

private:
    int numberOfChildrenInBVHTree(BVHTree& bvh);

};
#endif