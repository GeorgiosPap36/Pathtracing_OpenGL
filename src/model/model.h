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

struct Vertex{
    float x, y, z, nX, nY, nZ;

    Vertex() {
        x = y = z = nX = nY = nZ = 0;
    }

    Vertex(float x, float y, float z) : x(x), y(y), z(z) {
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