#ifndef MODEL_UTILS_H
#define MODEL_UTILS_H

#include <iostream>
#include <filesystem>
#include <fstream>

#include <glm/glm.hpp>

#include "model.h"

class ModelUtils {
public:
    ModelUtils();

    void printModelFileData(const char* modelFilePath);

    void printModelData(Model model);

    Model createModelFromPLY(const char* modelFilePath, bool containsNormals);

    void calculateAverageNormals(Model& model);
    
    void saveModel(Model model, const char* filepath);
};

#endif