#include "model_utils.h"

ModelUtils::ModelUtils() {

}

void ModelUtils::printModelFileData(const char* modelFilePath) {
    std::ifstream modelFile(modelFilePath);

    if (!modelFile.is_open()) {
        std::cerr << "Unable to open file: " << modelFilePath << std::endl;
    }

    std::string line;
    while (getline(modelFile, line)) {
        std::cout << line << std::endl;
    }

    modelFile.close();

}

void ModelUtils::printModelData(Model model) {
    for (int i = 0; i < model.vertices.size(); i++) {
        std::cout << model.vertices[i].x << " " << model.vertices[i].y << " " << model.vertices[i].z << " " 
        << model.vertices[i].nX << " " << model.vertices[i].nY << " " << model.vertices[i].nZ << std::endl; 
    }

    for (int i = 0; i < model.faces.size(); i++) {
        std::cout << model.faces[i].indices[0] << " " << model.faces[i].indices[1] << " " << model.faces[i].indices[2] << std::endl; 
    }
}

Model ModelUtils::createModelFromPLY(const char* modelFilePath, bool containsNormals) {
    Model model;
    std::ifstream modelFile(modelFilePath);

    if (!modelFile.is_open()) {
        std::cerr << "Unable to open file: " << modelFilePath << std::endl;
        return model;
    }

    std::string line;
    bool headerEnded = false;

    while (getline(modelFile, line)) {
        if (!headerEnded) {
            if (line == "end_header") {
                headerEnded = true;
            }
            continue;
        }

        std::istringstream ss(line);
        std::vector<double> values;
        double val;

        while (ss >> val) {
            values.push_back(val);
        }

        if (values[0] != 3) {
            Vertex vertex;
            vertex.x = values[0];
            vertex.y = values[1];
            vertex.z = values[2];

            if (containsNormals && values.size() >= 6) {
                vertex.nX = values[3];
                vertex.nY = values[4];
                vertex.nZ = values[5];
            }

            model.vertices.push_back(vertex);
        } else {
            Triangle triangle;
            triangle.indices[0] = static_cast<int>(values[1]);
            triangle.indices[1] = static_cast<int>(values[2]);
            triangle.indices[2] = static_cast<int>(values[3]);
            model.faces.push_back(triangle);
        }
    }

    modelFile.close();
    return model;
}

void ModelUtils::calculateAverageNormals(Model& model) {
    for (int i = 0; i < model.faces.size(); i++) {
        Triangle tr = model.faces[i];

        glm::vec3 edge1(model.vertices[tr.indices[1]].x - model.vertices[tr.indices[0]].x, 
                        model.vertices[tr.indices[1]].y - model.vertices[tr.indices[0]].y, 
                        model.vertices[tr.indices[1]].z - model.vertices[tr.indices[0]].z);
                
        glm::vec3 edge2(model.vertices[tr.indices[2]].x - model.vertices[tr.indices[0]].x, 
                        model.vertices[tr.indices[2]].y - model.vertices[tr.indices[0]].y, 
                        model.vertices[tr.indices[2]].z - model.vertices[tr.indices[0]].z);

        glm::vec3 normalVector = glm::cross(edge1, edge2);

        model.vertices[tr.indices[0]].nX += normalVector.x;
        model.vertices[tr.indices[0]].nY += normalVector.y;
        model.vertices[tr.indices[0]].nZ += normalVector.z;

        model.vertices[tr.indices[1]].nX += normalVector.x;
        model.vertices[tr.indices[1]].nY += normalVector.y;
        model.vertices[tr.indices[1]].nZ += normalVector.z;

        model.vertices[tr.indices[2]].nX += normalVector.x;
        model.vertices[tr.indices[2]].nY += normalVector.y;
        model.vertices[tr.indices[2]].nZ += normalVector.z;
    }

    for (int i = 0; i < model.vertices.size(); i++) {
        glm::vec3 normalVector(model.vertices[i].nX, model.vertices[i].nY, model.vertices[i].nZ);

        if (glm::length(normalVector) > 0.0f) {
            normalVector = glm::normalize(normalVector);
        }

        model.vertices[i].nX = normalVector.x;
        model.vertices[i].nY = normalVector.y;
        model.vertices[i].nZ = normalVector.z;
    }

}

void ModelUtils::saveModel(Model model, const char* filepath) {
    std::ofstream file;

    file.open(filepath);

    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << filepath << std::endl;
    }

    file << "ply\n";
    file << "end_header";

    for (int i = 0; i < model.vertices.size(); i++) {
        file << "\n";
        file << model.vertices[i].x << " " << model.vertices[i].y << " " << model.vertices[i].z << 
        " " << model.vertices[i].nX << " " << model.vertices[i].nY << " " << model.vertices[i].nZ;
    }

    for (int i = 0; i < model.faces.size(); i++) {
        file << "\n3 ";
        file << model.faces[i].indices[0] << " " << model.faces[i].indices[1] << " " << model.faces[i].indices[2];
    }

    file.close();
}

std::unique_ptr<BVHTree> ModelUtils::subdivideModel(glm::vec3 minPoint, glm::vec3 maxPoint,
                                   std::vector<std::array<int,3>>& modelFaces,
                                   std::vector<glm::vec3>& centroids,
                                   std::vector<glm::vec3>& modelVertices,
                                   std::vector<glm::vec4>& indices, 
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

void ModelUtils::addBVHTreeToBVHNodes(BVHTree& bvh, int missIndex, bool isRight, std::vector<BVHNode>& bvhNodes) {
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

int ModelUtils::numberOfChildrenInBVHTree(BVHTree& bvh) {
    BVHNode bvhNode = bvh.bvhNode;

    int sum = 1;

    if (!bvhNode.isLeaf) {
        sum += numberOfChildrenInBVHTree(*bvh.leftChild);
        sum += numberOfChildrenInBVHTree(*bvh.rightChild);
    } 

    return sum;
}
