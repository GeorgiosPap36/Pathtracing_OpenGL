#ifndef SCENE_H
#define SCENE_H

#include <iostream>
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "compute_shader.h"

#include "model/model.h"
#include "model/model_utils.h"
#include "model/bvh_utils.h"


struct alignas(16) Sphere {
    glm::vec3 center;
    float radius;
    Material material;
                            
    Sphere(glm::vec3 center, float radius, Material material) : 
        center(center), radius(radius), material(material) {

    }
};

class Scene {

public:
    ComputeShader computeShader;

    Scene(ComputeShader computeShader, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT);
    ~Scene();
    GLuint renderScene(glm::vec3 cameraPos, glm::mat4x4 viewMatrix, bool accumulateFrames, int frameCounter);

private:
    GLuint sphereSSBO;
    GLuint vertexSSBO;
    GLuint indexSSBO;
    GLuint materialSSBO;
    GLuint bvhNodeSSBO;
    GLuint modelInfoSSBO;
    GLuint thisFrameTex;
    GLuint lastFrameTex;

    std::vector<Sphere> spheres;
    std::vector<Vertex> vertices;
    std::vector<glm::ivec4> indices;
    std::vector<Material> materials;
    std::vector<BVHNode> bvhNodes;
    std::vector<ModelInfo> modelInfos;

    unsigned int SCR_WIDTH;
    unsigned int SCR_HEIGHT;

    void addQuad(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 normal, Material material);
    void addModel(const char* modelFilePath, glm::vec3 offset, float scale, float angle, Material material, int maximumNumberOfFacesPerNode);
    void createCornellBox(glm::vec3 center, glm::vec3 size, std::vector<Material> materials);
    void createSSBOs();

    // Scenes
    void testScene();
    void testScene2();
};


#endif