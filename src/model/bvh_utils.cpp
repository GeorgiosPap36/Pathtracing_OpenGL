#include "bvh_utils.h"

std::unique_ptr<BVHTree> BVHUtils::subdivideModel(glm::vec3 minPoint, glm::vec3 maxPoint,
                                   std::vector<std::array<int,3>>& modelFaces,
                                   std::vector<glm::vec3>& centroids,
                                   std::vector<glm::vec3>& modelVertices,
                                   std::vector<glm::ivec4>& indices, 
                                   int numberOfFacesInLeaves) {

    std::unique_ptr<BVHTree> bvh = std::make_unique<BVHTree>(BVHNode(minPoint, maxPoint, false, -1, 0, 0));
    
    glm::vec3 aabbSize = maxPoint - minPoint;

    if (modelFaces.size() <= numberOfFacesInLeaves || glm::all(glm::lessThanEqual(aabbSize, glm::vec3(1e-5f)))) {
        bvh->bvhNode.isLeaf = true;

        bvh->bvhNode.firstFaceIndex = indices.size();
        for (auto f : modelFaces) {
            indices.push_back(glm::vec4(f[0], f[1], f[2], 0));
        }
        bvh->bvhNode.lastFaceIndex = indices.size() - 1;

        return bvh;
    }

    float maxAxis = std::max(aabbSize.x, std::max(aabbSize.y, aabbSize.z));

    char axis = (maxAxis == aabbSize.x) ? 'X' :
                (maxAxis == aabbSize.y) ? 'Y' : 'Z';

    glm::vec3 aabbCenter = minPoint + aabbSize / 2.0f;

    std::vector<std::array<int,3>> modelFacesLeft, modelFacesRight;
    std::vector<glm::vec3> centroidsLeft, centroidsRight;

    for (int i = 0; i < modelFaces.size(); i++) {
        glm::vec3 c = centroids[i];

        bool right = (axis == 'X' && c.x > aabbCenter.x) ||
                     (axis == 'Y' && c.y > aabbCenter.y) ||
                     (axis == 'Z' && c.z > aabbCenter.z);

        if (right) {
            modelFacesRight.push_back(modelFaces[i]);
            centroidsRight.push_back(c);
        } else {
            modelFacesLeft.push_back(modelFaces[i]);
            centroidsLeft.push_back(c);
        }
    }

    if (modelFacesLeft.empty() && !modelFacesRight.empty()) {
        modelFacesLeft.push_back(modelFacesRight.back());
        centroidsLeft.push_back(centroidsRight.back());
        modelFacesRight.pop_back();
        centroidsRight.pop_back();
    } else if (modelFacesRight.empty() && !modelFacesLeft.empty()) {
        modelFacesRight.push_back(modelFacesLeft.back());
        centroidsRight.push_back(centroidsLeft.back());
        modelFacesLeft.pop_back();
        centroidsLeft.pop_back();
    }

    glm::vec3 minLeft(1e+5f), maxLeft(-1e+5f);
    glm::vec3 minRight(1e+5f), maxRight(-1e+5f);

    for (auto f : modelFacesLeft) {
        for (int j = 0; j < 3; j++) {
            glm::vec3 pos = modelVertices[f[j]];
            minLeft = glm::min(minLeft, pos);
            maxLeft = glm::max(maxLeft, pos);
        }
    }

    for (std::array<int,3> f : modelFacesRight) {
        for (int j = 0; j < 3; j++) {
            glm::vec3 pos = modelVertices[f[j]];
            minRight = glm::min(minRight, pos);
            maxRight = glm::max(maxRight, pos);
        }
    }

    if (!modelFacesRight.empty()) {
        bvh->rightChild = subdivideModel(minRight, maxRight, modelFacesRight, centroidsRight, modelVertices, indices, numberOfFacesInLeaves);
    }

    if (!modelFacesLeft.empty()) {
        bvh->leftChild = subdivideModel(minLeft, maxLeft, modelFacesLeft, centroidsLeft, modelVertices, indices, numberOfFacesInLeaves);
    }

    return bvh;
}

void BVHUtils::addBVHTreeToBVHNodes(BVHTree& bvh, int missIndex, bool isRight, std::vector<BVHNode>& bvhNodes) {
    BVHNode bvhNode = bvh.bvhNode;

    if (!bvh.isRoot) {
        bvhNode.missIndex = isRight ? missIndex : (numberOfChildrenInBVHTree(bvh) + bvhNodes.size());
        // std::cout << "bvhNode.missIndex: " << bvhNode.missIndex << std::endl;
    }

    bvhNodes.push_back(bvhNode);

    if (bvhNode.isLeaf) {
        return;
    }

    addBVHTreeToBVHNodes(*bvh.leftChild, bvhNode.missIndex, false, bvhNodes);
    addBVHTreeToBVHNodes(*bvh.rightChild, bvhNode.missIndex, true, bvhNodes);
}

int BVHUtils::numberOfChildrenInBVHTree(BVHTree& bvh) {
    BVHNode bvhNode = bvh.bvhNode;

    int sum = 1;

    if (!bvhNode.isLeaf) {
        sum += numberOfChildrenInBVHTree(*bvh.leftChild);
        sum += numberOfChildrenInBVHTree(*bvh.rightChild);
    } 

    return sum;
}
