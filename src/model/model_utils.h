#ifndef MODEL_UTILS_H
#define MODEL_UTILS_H

#include <iostream>
#include <filesystem>
#include <fstream>
#include <array>

#include <glm/glm.hpp>

#include "model.h"

struct BVHTree {
    BVHNode bvhNode;
    std::unique_ptr<BVHTree> leftChild;
    std::unique_ptr<BVHTree> rightChild;
    bool isRoot;

    BVHTree(BVHNode bvhNode) : bvhNode(bvhNode) {
        isRoot = false;
    }
};

class ModelUtils {
public:
    ModelUtils();

    void printModelFileData(const char* modelFilePath);

    void printModelData(Model model);

    Model createModelFromPLY(const char* modelFilePath, bool containsNormals);

    void calculateAverageNormals(Model& model);
    
    void saveModel(Model model, const char* filepath);

    std::unique_ptr<BVHTree> subdivideModel(glm::vec3 minPoint, glm::vec3 maxPoint,
                           std::vector<std::array<int,3>>& modelFaces,
                           std::vector<glm::vec3>& centroids,
                           std::vector<glm::vec3>& modelVertices,
                           std::vector<glm::vec4>& indices, 
                           int numberOfFacesInLeaves);

    void addBVHTreeToBVHNodes(BVHTree& bvh, int missIndex, bool isRight, std::vector<BVHNode>& bvhNodes);

private:
    int numberOfChildrenInBVHTree(BVHTree& bvh);
};

#endif