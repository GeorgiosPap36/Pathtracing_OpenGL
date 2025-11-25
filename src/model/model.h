#ifndef MODEL_H
#define MODEL_H

#include <iostream>
#include <vector>

#include "../shader.h"

struct Triangle{
    int indices[3];

    Triangle() {
        indices[0] = indices[1] = indices[2] = 0;
    }
};

struct alignas(16) Vertex{
    float x, y, z, pad1, nX, nY, nZ, pad2;

    Vertex() {
        x = y = z = nX = nY = nZ = 0;
    }

    Vertex(float x, float y, float z) : x(x), y(y), z(z) {
    }

    Vertex(glm::vec3 pos, glm::vec3 normal) 
        : x(pos.x), y(pos.y), z(pos.z), nX(normal.x), nY(normal.y), nZ(normal.z) {
    }

    Vertex(float x, float y, float z, float nX, float nY, float nZ) 
        : x(x), y(y), z(z), nX(nX), nY(nY), nZ(nZ) {
    }
};

struct alignas(16) Material {
    glm::vec3 color;
    float smoothness;
    glm::vec3 emissionColor;
    float emissionStrength;
    float refractionProbability;
    float refractionIndex;
                             
    Material(glm::vec3  color, float smoothness, glm::vec3 emissionColor, float emissionStrength, float refractionProbability, float refractionIndex) : 
        color(color), smoothness(smoothness), 
        emissionColor(emissionColor), emissionStrength(emissionStrength), 
        refractionProbability(refractionProbability), refractionIndex(refractionIndex) {

    }
};

struct ModelInfo {
    int vertexCount;
    int indexCount;
    int materialIndex;
    int bvhNodeFirstIndex;
    int bvhNodeLastIndex;

    ModelInfo(int vertexCount, int indexCount, int materialIndex, int bvhNodeFirstIndex, int bvhNodeLastIndex) : 
        vertexCount(vertexCount), indexCount(indexCount), materialIndex(materialIndex), bvhNodeFirstIndex(bvhNodeFirstIndex), bvhNodeLastIndex(bvhNodeLastIndex) {

    }
};

class Model {
public:
    std::vector<Vertex> vertices;
    std::vector<Triangle> faces;

    unsigned int VAO, VBO, EBO;

    Model();

    void transferDataToGPU();
    void draw(Shader shader);
};

#endif