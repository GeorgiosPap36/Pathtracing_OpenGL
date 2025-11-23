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