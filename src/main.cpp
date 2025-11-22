#include <iostream>
#include <filesystem>

#include "../dependencies/glad.h"
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../dependencies/stb_image.h"

#include "shader.h"
#include "compute_shader.h"
#include "camera.h"

#include "model/model_utils.h"
#include "model/model.h"



struct alignas(16) Material {
    glm::vec3 color;
    float smoothness;
    glm::vec3 emissionColor;
    float luminosity;
    float specularProbability;
                             
    Material(glm::vec3  color, float smoothness, glm::vec3 emissionColor, float luminosity, float specularProbability) : 
        color(color), smoothness(smoothness), emissionColor(emissionColor), luminosity(luminosity), specularProbability(specularProbability) {

    }
};

struct alignas(16) Sphere {
    glm::vec3 center;
    float radius;
    Material material;
                            
    Sphere(glm::vec3 center, float radius, Material material) : 
        center(center), radius(radius), material(material) {

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


// functions
void addQuad(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 normal, Material material);
void addModel(const char* modelFilePath, glm::vec3 offset, float scale, Material material);
void createCornellBox(glm::vec3 center, glm::vec3 size);
void renderRaytracingQuad(Shader shader, GLuint screenTex);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0, 0, 2.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

bool accumulateFrames = false;
int frameCounter = 0;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;


std::vector<Sphere> spheres;
std::vector<Vertex> vertices;
std::vector<glm::vec4> indices;
std::vector<Material> materials;
std::vector<BVHNode> bvhNodes;
std::vector<ModelInfo> modelInfos;

int main() {
    glfwInit();
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Raytracing", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // Face culling 
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    Shader debugModelsShader("../shaders/simpleVertexShader.vert", "../shaders/simpleFragmentShader.frag");
    Shader renderRayTracingTextureShader("../shaders/raytracingVertexShader.vert", "../shaders/raytracingFragmentShader.frag");

    ComputeShader pathTracingComputeShader("../shaders/pathTracingShader.comp");

    // Sphere s(glm::vec3(0, 2, 1), 0.3, Material(glm::vec3(1), 0.75, glm::vec3(1), 0, 1));
    // spheres.push_back(s);
    // Sphere s1(glm::vec3(0, 0.75, 0.25), 0.1f, Material(glm::vec3(0.25, 0, 1), 0, glm::vec3(1), 0, 0));
    // spheres.push_back(s1);
    //
    // Sphere lightSphere(glm::vec3(0, 50, -100), 50, Material(glm::vec3(1, 1, 1), 0, glm::vec3(1), 1, 0));
    // spheres.push_back(lightSphere);
    // Sphere lightSphere2(glm::vec3(0, 100, 125), 75, Material(glm::vec3(1, 0.5, 0.3), 0, glm::vec3(1), 1, 0));
    // spheres.push_back(lightSphere2);
    //
    // Sphere floorS(glm::vec3(0, -100, 0), 100, Material(glm::vec3(1, 1, 1), 0.25, glm::vec3(1), 0, 1));
    // spheres.push_back(floorS);
    //
    // Sphere floor1(glm::vec3(2, 4, 5), 0.1, Material(glm::vec3(1, 0, 1), 0, glm::vec3(1), 0, 1));
    // spheres.push_back(floor1);
    // Sphere floor2(glm::vec3(2, 4, 10), 0.1, Material(glm::vec3(0, 1, 0), 0, glm::vec3(1), 0, 1));
    // spheres.push_back(floor2);
    // Sphere floor3(glm::vec3(2, 10, 5), 0.1, Material(glm::vec3(0, 0, 1), 0, glm::vec3(1), 0, 1));
    // spheres.push_back(floor3);
    // Sphere floor4(glm::vec3(2, 10, 10), 0.1, Material(glm::vec3(1, 1, 1), 0, glm::vec3(1), 0, 1));
    // spheres.push_back(floor4);

    createCornellBox(glm::vec3(0), glm::vec3(5));
    addModel("../assets/models/bunny/bun_res4_normals.ply", glm::vec3(0), 8, Material(glm::vec3(1, 1, 1), 0, glm::vec3(1), 0, 1));

    GLuint sphereSSBO;
    glGenBuffers(1, &sphereSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Sphere) * spheres.size(), spheres.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sphereSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLuint vertexSSBO;
    glGenBuffers(1, &vertexSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, vertexSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLuint indexSSBO;
    glGenBuffers(1, &indexSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * indices.size(), indices.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, indexSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLuint materialSSBO;
    glGenBuffers(1, &materialSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Material) * materials.size(), materials.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, materialSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLuint bvhNodeSSBO;
    glGenBuffers(1, &bvhNodeSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvhNodeSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BVHNode) * bvhNodes.size(), bvhNodes.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, bvhNodeSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLuint modelInfoSSBO;
    glGenBuffers(1, &modelInfoSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, modelInfoSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ModelInfo) * modelInfos.size(), modelInfos.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, modelInfoSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    pathTracingComputeShader.use();

    pathTracingComputeShader.setVec3("cameraPosition", camera.Position);
    pathTracingComputeShader.setMat4("viewMatrix", camera.getViewMatrix());

    pathTracingComputeShader.setInt("width", SCR_WIDTH);
    pathTracingComputeShader.setInt("height", SCR_HEIGHT);
    pathTracingComputeShader.setInt("numberOfSpheres", spheres.size());
    pathTracingComputeShader.setInt("numberOfModels", modelInfos.size());

    pathTracingComputeShader.setInt("frameCounter", frameCounter);
    pathTracingComputeShader.setBool("accumulateFrames", accumulateFrames);

    GLuint thisFrameTex;
    glGenTextures(1, &thisFrameTex);
    glBindTexture(GL_TEXTURE_2D, thisFrameTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindImageTexture(0, thisFrameTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    GLuint lastFrameTex;
    glGenTextures(1, &lastFrameTex);
    glBindTexture(GL_TEXTURE_2D, lastFrameTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindImageTexture(1, lastFrameTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

    pathTracingComputeShader.dispatch(SCR_WIDTH / 8, SCR_HEIGHT / 8, 1);

    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSSBO);
    // float* ptr = (float*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    // if (ptr) {
    //     for (int i = 0; i < 10; i++)
    //         std::cout << "Result[" << i << "] = " << ptr[i] << std::endl;
    //     glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    // }
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    // glDeleteBuffers(1, &sphereSSBO);


    // FPS variables
    double prevTime = 0.0f;
    double crntTime = 0.0f;
    double timeDiff;
    unsigned int counter = 0;

    while (!glfwWindowShouldClose(window)) {
        // FPS counter
        crntTime = glfwGetTime();
        timeDiff = crntTime - prevTime;
        counter++;
        if (timeDiff >= 10.0 / 30.0) {
            std::string FPS = std::to_string((1.0 / timeDiff) * counter);
            std::string ms = std::to_string((timeDiff / counter) * 1000);
            std::string newTitle = "Raytracing - " + FPS + "FPS / " + ms + "ms";
            glfwSetWindowTitle(window, newTitle.c_str());
            prevTime = crntTime;
            counter = 0;
        }

        // inputs
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        // debugModelsShader.setMat4("projection", projection);
        // debugModelsShader.setMat4("view", view);
        // debugModelsShader.setMat4("model", model);
        // mod.draw(debugModelsShader);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sphereSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, vertexSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, indexSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, materialSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, bvhNodeSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, modelInfoSSBO);

        pathTracingComputeShader.use();

        pathTracingComputeShader.setVec3("cameraPosition", camera.Position);
        pathTracingComputeShader.setMat4("viewMatrix", view);

        pathTracingComputeShader.setInt("width", SCR_WIDTH);
        pathTracingComputeShader.setInt("height", SCR_HEIGHT);
        pathTracingComputeShader.setInt("numberOfSpheres", spheres.size());
        pathTracingComputeShader.setInt("numberOfModels", modelInfos.size());

        pathTracingComputeShader.setInt("frameCounter", frameCounter);
        pathTracingComputeShader.setBool("accumulateFrames", accumulateFrames);

        if (accumulateFrames) {
            GLuint tempFrame = thisFrameTex;
            thisFrameTex = lastFrameTex;
            lastFrameTex = tempFrame;
            frameCounter++;
        }
        
        glBindImageTexture(0, thisFrameTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glBindImageTexture(1, lastFrameTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

        pathTracingComputeShader.dispatch(SCR_WIDTH / 8, SCR_HEIGHT / 8, 1);

        renderRaytracingQuad(renderRayTracingTextureShader, thisFrameTex);

        // swap buffers, do events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(debugModelsShader.ID);
    glDeleteProgram(pathTracingComputeShader.ID);
    glDeleteProgram(renderRayTracingTextureShader.ID);

    glfwTerminate();
    return 0;
}

void createCornellBox(glm::vec3 center, glm::vec3 size) {

    glm::vec3 a = center + glm::vec3(-size.x, -size.y, -size.z) / 2.0f;
    glm::vec3 b = center + glm::vec3(-size.x, -size.y,  size.z) / 2.0f;
    glm::vec3 c = center + glm::vec3( size.x, -size.y,  size.z) / 2.0f;
    glm::vec3 d = center + glm::vec3( size.x, -size.y, -size.z) / 2.0f;
    glm::vec3 e = center + glm::vec3(-size.x,  size.y, -size.z) / 2.0f;
    glm::vec3 f = center + glm::vec3(-size.x,  size.y,  size.z) / 2.0f;
    glm::vec3 g = center + glm::vec3( size.x,  size.y,  size.z) / 2.0f;
    glm::vec3 h = center + glm::vec3( size.x,  size.y, -size.z) / 2.0f;

    glm::vec3 normalLeft   = glm::vec3(-1,  0,  0);
    glm::vec3 normalRight  = glm::vec3( 1,  0,  0);
    glm::vec3 normalBottom = glm::vec3( 0, -1,  0);
    glm::vec3 normalTop    = glm::vec3( 0,  1,  0);
    glm::vec3 normalBack   = glm::vec3( 0,  0, -1);
    glm::vec3 normalFront  = glm::vec3( 0,  0,  1);

    addQuad(a, b, f, e, normalRight, Material(glm::vec3(1, 0, 0), 0, glm::vec3(1), 0, 1));  // left
    addQuad(d, c, g, h, normalLeft, Material(glm::vec3(0, 1, 0), 0, glm::vec3(1), 0, 1));   // right
    addQuad(a, d, c, b, normalTop, Material(glm::vec3(1, 1, 1), 0, glm::vec3(1), 0, 1));    // bottom
    addQuad(e, f, g, h, normalBottom, Material(glm::vec3(1, 1, 1), 0, glm::vec3(1), 0, 1)); // top
    addQuad(a, e, h, d, normalFront, Material(glm::vec3(0, 0, 1), 0, glm::vec3(1), 0, 1));  // back
    addQuad(b, c, g, f, normalBack, Material(glm::vec3(1, 1, 1), 0, glm::vec3(1), 0, 1));   // front

    glm::vec3 lightE = glm::vec3(e.x / 3.0f, e.y - 0.00001, e.z / 3.0f);
    glm::vec3 lightF = glm::vec3(f.x / 3.0f, f.y - 0.00001, f.z / 3.0f);
    glm::vec3 lightG = glm::vec3(g.x / 3.0f, g.y - 0.00001, g.z / 3.0f);
    glm::vec3 lightH = glm::vec3(h.x / 3.0f, h.y - 0.00001, h.z / 3.0f);

    addQuad(lightE, lightF, lightG, lightH, normalBottom, Material(glm::vec3(1, 1, 1), 0, glm::vec3(1), 5, 1));   // light

    // Sphere sphere1(glm::vec3(0, 0, 0), 1, Material(glm::vec3(0.5), 0.95, glm::vec3(0), 0, 1));
    // spheres.push_back(sphere1);

    // Sphere aS(a, 0.1, Material(glm::vec3(1, 0, 1), 0, glm::vec3(1), 1, 1));
    // spheres.push_back(aS);
    // Sphere bS(b, 0.1, Material(glm::vec3(1, 0, 1), 0, glm::vec3(1), 1, 1));
    // spheres.push_back(bS);
    // Sphere cS(c, 0.1, Material(glm::vec3(1, 0, 1), 0, glm::vec3(1), 1, 1));
    // spheres.push_back(cS);
    // Sphere dS(d, 0.1, Material(glm::vec3(1, 0, 1), 0, glm::vec3(1), 1, 1));
    // spheres.push_back(dS);
    // Sphere eS(e, 0.1, Material(glm::vec3(1, 0, 1), 0, glm::vec3(1), 1, 1));
    // spheres.push_back(eS);
    // Sphere fS(f, 0.1, Material(glm::vec3(1, 0, 1), 0, glm::vec3(1), 1, 1));
    // spheres.push_back(fS);
    // Sphere gS(g, 0.1, Material(glm::vec3(1, 0, 1), 0, glm::vec3(1), 1, 1));
    // spheres.push_back(gS);
    // Sphere hS(h, 0.1, Material(glm::vec3(1, 0, 1), 0, glm::vec3(1), 1, 1));
    // spheres.push_back(hS);
}

void addQuad(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 normal, Material material) {

    vertices.push_back(Vertex(v0, normal));
    vertices.push_back(Vertex(v1, normal));
    vertices.push_back(Vertex(v2, normal));
    vertices.push_back(Vertex(v3, normal));

    float minX = glm::min(v0.x, glm::min(v1.x, glm::min(v2.x, v3.x)));
    float minY = glm::min(v0.y, glm::min(v1.y, glm::min(v2.y, v3.y)));
    float minZ = glm::min(v0.z, glm::min(v1.z, glm::min(v2.z, v3.z)));

    float maxX = glm::max(v0.x, glm::max(v1.x, glm::max(v2.x, v3.x)));
    float maxY = glm::max(v0.y, glm::max(v1.y, glm::max(v2.y, v3.y)));
    float maxZ = glm::max(v0.z, glm::max(v1.z, glm::max(v2.z, v3.z)));
 
    int firstFaceIndex = indices.size();
    indices.push_back(glm::vec4(0, 1, 2, 0));
    int lastFaceIndex = indices.size();
    indices.push_back(glm::vec4(0, 2, 3, 0));

    // std::cout << faces[0] << ", " << faces[1] << std::endl;

    int bvhNodeIndex = bvhNodes.size();
    BVHNode bvhNode(glm::vec3(minX, minY, minZ), glm::vec3(maxX, maxY, maxZ), true, -1, firstFaceIndex, lastFaceIndex);
    bvhNodes.push_back(bvhNode);

    int matIndex = materials.size();
    materials.push_back(material);

    modelInfos.push_back(ModelInfo(4, 2, matIndex, bvhNodeIndex, bvhNodeIndex));
}

void addModel(const char* modelFilePath, glm::vec3 offset, float scale, Material material) {

    ModelUtils modelUtils;
    // Model model = modelUtils.createModelFromPLY(modelFilePath, false);
    // modelUtils.calculateAverageNormals(model);
    // model.transferDataToGPU();
    // modelUtils.saveModel(model, "../assets/models/bunny/bun_res4_normals.ply");
    
    Model mod = modelUtils.createModelFromPLY(modelFilePath, true);

    materials.push_back(material);

    float minX = 1000000;
    float minY = 1000000;
    float minZ = 1000000;

    float maxX = -1000000;
    float maxY = -1000000;
    float maxZ = -1000000;

    std::vector<glm::vec3> modifiedVertexPositions;
    
    for (int i = 0; i < mod.vertices.size(); i++) {
        glm::vec3 normal = glm::vec3(mod.vertices[i].nX, mod.vertices[i].nY, mod.vertices[i].nZ);
        glm::vec3 pos = glm::vec3(mod.vertices[i].x, mod.vertices[i].y, mod.vertices[i].z);
        pos = (pos + offset) * scale;

        minX = glm::min(pos.x, minX);
        minY = glm::min(pos.y, minY);
        minZ = glm::min(pos.z, minZ);

        maxX = glm::max(pos.x, maxX);
        maxY = glm::max(pos.y, maxY);
        maxZ = glm::max(pos.z, maxZ);

        vertices.push_back(Vertex(pos, normal));
        modifiedVertexPositions.push_back(pos);
    }

    int firstFaceIndex = indices.size();

    std::vector<std::array<int,3>> modelFaces;
    std::vector<glm::vec3> centroids;

    for (int i = 0; i < mod.faces.size(); i++) {
        // indices.push_back(glm::vec4(mod.faces[i].indices[0], mod.faces[i].indices[1], mod.faces[i].indices[2], 0));

        int index0 = mod.faces[i].indices[0];
        int index1 = mod.faces[i].indices[1];
        int index2 = mod.faces[i].indices[2];

        std::array<int,3> modelIndices = {index0, index1, index2};

        modelFaces.push_back(modelIndices);

        float x0 = modifiedVertexPositions[index0].x;
        float x1 = modifiedVertexPositions[index1].x;
        float x2 = modifiedVertexPositions[index2].x;

        float y0 = modifiedVertexPositions[index0].y;
        float y1 = modifiedVertexPositions[index1].y;
        float y2 = modifiedVertexPositions[index2].y;

        float z0 = modifiedVertexPositions[index0].z;
        float z1 = modifiedVertexPositions[index1].z;
        float z2 = modifiedVertexPositions[index2].z;

        centroids.push_back((glm::vec3(x0, y0, z0) + glm::vec3(x1, y1, z1) + glm::vec3(x2, y2, z2)) * 0.3333f);
    }

    std::cout << modifiedVertexPositions.size() << ", " << centroids.size() << ", " << modelFaces.size() << std::endl;

    int lastFaceIndex = indices.size() - 1;

    int bvhNodeIndex = bvhNodes.size();

    auto bvh = modelUtils.subdivideModel(glm::vec3(minX, minY, minZ), glm::vec3(maxX, maxY, maxZ), 
                                        modelFaces, centroids, modifiedVertexPositions, indices, 5);

    BVHTree& bvhTree = *bvh; 
    bvhTree.isRoot = true;
    modelUtils.addBVHTreeToBVHNodes(bvhTree, -1, false, bvhNodes);

    ModelInfo modModelInfo(mod.vertices.size(), mod.faces.size(), modelInfos.size(), bvhNodeIndex, bvhNodes.size() - 1);
    modelInfos.push_back(modModelInfo);

    // mod.transferDataToGPU();
}

void renderRaytracingQuad(Shader shader, GLuint screenTex) {
     static unsigned int quadVAO = 0, quadVBO = 0, quadEBO = 0;
    if (quadVAO == 0) {
        float quadVertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f, 1.0f
        };
        unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glGenBuffers(1, &quadEBO);

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    }

    shader.use();
    shader.setInt("screenTexture", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screenTex);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(RIGHT, deltaTime);

    static bool fKeyPressed = false;

    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (!fKeyPressed) {
            if (!accumulateFrames) {
                frameCounter = 0;
            }

            accumulateFrames = !accumulateFrames;
            fKeyPressed = true;
            std::cout << "Accumulate frames = " << accumulateFrames << std::endl;
        }
    } else {
        fKeyPressed = false;
    }
}

void mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.processMouseMovement(xoffset, yoffset);
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}