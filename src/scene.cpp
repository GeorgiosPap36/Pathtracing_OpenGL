#include "scene.h"


Scene::Scene(ComputeShader computeShader, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT) : 
    computeShader(computeShader), SCR_WIDTH(SCR_WIDTH), SCR_HEIGHT(SCR_HEIGHT) {
    
    // testScene();
    // testScene2();
    mirrorsEveryWhere();
    
    createSSBOs();
}

Scene::~Scene() {
    glDeleteBuffers(1, &sphereSSBO);
    glDeleteBuffers(1, &vertexSSBO); 
    glDeleteBuffers(1, &indexSSBO); 
    glDeleteBuffers(1, &materialSSBO); 
    glDeleteBuffers(1, &modelInfoSSBO); 
    glDeleteBuffers(1, &thisFrameTex); 
    glDeleteBuffers(1, &lastFrameTex);
}

GLuint Scene::renderScene(glm::vec3 cameraPos, glm::mat4x4 viewMatrix, bool accumulateFrames, int frameCounter) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sphereSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, vertexSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, indexSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, materialSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, bvhNodeSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, modelInfoSSBO);

    computeShader.use();

    computeShader.setVec3("cameraPosition", cameraPos);
    computeShader.setMat4("viewMatrix", viewMatrix);

    computeShader.setInt("width", SCR_WIDTH);
    computeShader.setInt("height", SCR_HEIGHT);
    computeShader.setInt("numberOfSpheres", spheres.size());
    computeShader.setInt("numberOfModels", modelInfos.size());

    computeShader.setInt("frameCounter", frameCounter);
    computeShader.setBool("accumulateFrames", accumulateFrames);

    if (accumulateFrames) {
        GLuint tempFrame = thisFrameTex;
        thisFrameTex = lastFrameTex;
        lastFrameTex = tempFrame;
    }
    
    glBindImageTexture(0, thisFrameTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(1, lastFrameTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

    computeShader.dispatch(SCR_WIDTH / 8, SCR_HEIGHT / 8, 1);
    
    return thisFrameTex;
}

void Scene::addQuad(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 normal, Material material) {
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
    indices.push_back(glm::vec4(0, 2, 1, 0));
    int lastFaceIndex = indices.size();
    indices.push_back(glm::vec4(0, 3, 2, 0));

    // std::cout << faces[0] << ", " << faces[1] << std::endl;

    int bvhNodeIndex = bvhNodes.size();
    BVHNode bvhNode(glm::vec3(minX, minY, minZ), glm::vec3(maxX, maxY, maxZ), true, -1, firstFaceIndex, lastFaceIndex);
    bvhNodes.push_back(bvhNode);

    int matIndex = materials.size();
    materials.push_back(material);

    modelInfos.push_back(ModelInfo(4, 2, matIndex, bvhNodeIndex, bvhNodeIndex));
}

void Scene::addModel(const char* modelFilePath, glm::vec3 offset, float scale, float angle, Material material, int maximumNumberOfFacesPerNode) {
    
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

    float angleRadians = glm::radians(angle);
    glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);

    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angleRadians, rotationAxis);
    
    for (int i = 0; i < mod.vertices.size(); i++) {
        glm::vec3 normal = glm::vec3(mod.vertices[i].nX, mod.vertices[i].nY, mod.vertices[i].nZ);
        glm::vec3 pos = glm::vec3(mod.vertices[i].x, mod.vertices[i].y, mod.vertices[i].z);
        pos = glm::vec3(rotationMatrix * glm::vec4(pos, 1.0f));
        pos = (pos + offset) * scale;

        normal = glm::vec3(rotationMatrix * glm::vec4(normal, 0.0f)); // rotate normals too

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

    std::cout << "Number of vertices: " << modifiedVertexPositions.size() << std::endl;
    std::cout << "Number of faces: " << modelFaces.size() << std::endl;

    int lastFaceIndex = indices.size() - 1;

    int bvhNodeIndex = bvhNodes.size();

    BVHUtils bvhUtils;

    auto bvh = bvhUtils.subdivideModel(glm::vec3(minX, minY, minZ), glm::vec3(maxX, maxY, maxZ), 
                                        modelFaces, centroids, modifiedVertexPositions, indices, maximumNumberOfFacesPerNode);

    BVHTree& bvhTree = *bvh; 
    bvhTree.isRoot = true;
    bvhUtils.addBVHTreeToBVHNodes(bvhTree, -1, false, bvhNodes);

    ModelInfo modModelInfo(mod.vertices.size(), mod.faces.size(), modelInfos.size(), bvhNodeIndex, bvhNodes.size() - 1);
    modelInfos.push_back(modModelInfo);

    //empty modifiedVertexPositions, bvhTree, modelFaces, centroids

    // mod.transferDataToGPU();
}

void Scene::createCornellBox(glm::vec3 center, glm::vec3 size, std::vector<Material> materials) {
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

    addQuad(a, b, f, e, normalLeft, materials[0]);   // left
    addQuad(d, h, g, c, normalRight, materials[1]);  // right
    addQuad(a, d, c, b, normalBottom, materials[2]); // bottom
    addQuad(e, f, g, h, normalTop, materials[3]);    // top
    addQuad(a, e, h, d, normalBack, materials[4]);   // back
    addQuad(b, c, g, f, normalFront, materials[5]);  // front

    glm::vec3 lightE = glm::vec3(e.x / 3.0f, e.y - 0.00001, e.z / 3.0f);
    glm::vec3 lightF = glm::vec3(f.x / 3.0f, f.y - 0.00001, f.z / 3.0f);
    glm::vec3 lightG = glm::vec3(g.x / 3.0f, g.y - 0.00001, g.z / 3.0f);
    glm::vec3 lightH = glm::vec3(h.x / 3.0f, h.y - 0.00001, h.z / 3.0f);

    addQuad(lightE, lightF, lightG, lightH, normalBottom, Material(glm::vec3(1), 0, glm::normalize(glm::vec3(1)), 10, 0, 1)); // light
    
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

void Scene::createSSBOs() {
    glGenBuffers(1, &sphereSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Sphere) * spheres.size(), spheres.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sphereSSBO);

    glGenBuffers(1, &vertexSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, vertexSSBO);

    glGenBuffers(1, &indexSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * indices.size(), indices.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, indexSSBO);

    glGenBuffers(1, &materialSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Material) * materials.size(), materials.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, materialSSBO);

    glGenBuffers(1, &bvhNodeSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvhNodeSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BVHNode) * bvhNodes.size(), bvhNodes.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, bvhNodeSSBO);

    glGenBuffers(1, &modelInfoSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, modelInfoSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ModelInfo) * modelInfos.size(), modelInfos.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, modelInfoSSBO);

    glGenTextures(1, &thisFrameTex);
    glBindTexture(GL_TEXTURE_2D, thisFrameTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);

    glGenTextures(1, &lastFrameTex);
    glBindTexture(GL_TEXTURE_2D, lastFrameTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
}


// Scenes
void Scene::testScene() {
    std::vector<Material> materials;
    Material left(glm::vec3(1, 0, 0), 0, glm::vec3(0), 0, 0, 1);  
    materials.push_back(left);
    Material right(glm::vec3(0, 1, 0), 0, glm::vec3(0), 0, 0, 1);   
    materials.push_back(right);
    Material bottom(glm::vec3(1, 1, 1), 0, glm::vec3(0), 0, 0, 1);  
    materials.push_back(bottom);
    Material top(glm::vec3(1, 1, 1), 0, glm::vec3(0), 0, 0, 1);     
    materials.push_back(top);
    Material back(glm::vec3(1, 1, 1), 0, glm::vec3(0), 0, 0, 1);   
    materials.push_back(back); 
    Material front(glm::vec3(1, 1, 1), 0, glm::vec3(0), 0, 0, 1);   
    materials.push_back(front);


    createCornellBox(glm::vec3(0), glm::vec3(5), materials);
    // addModel("../assets/models/bunny/bun_res4_normals.ply", glm::vec3(0, 0, 0), 10, 30, mat, 2);
    // addModel("../assets/models/cube.ply", glm::vec3(0, 0.5, 0), 0.5, 0, mat, 2);
    
    Sphere sphere1(glm::vec3(-1, -0.5, 0), 0.5, Material(glm::vec3(1), 1, glm::normalize(glm::vec3(1)), 0, glm::vec3(0), 0, 1, 1.25));
    spheres.push_back(sphere1);
}

void Scene::testScene2() {
    std::vector<Material> materials;
    Material left(glm::vec3(1), 0, glm::vec3(0), 0, 0, 1);  
    materials.push_back(left);
    Material right(glm::vec3(1), 0, glm::vec3(0), 0, 0, 1);   
    materials.push_back(right);
    Material bottom(glm::vec3(1), 0, glm::vec3(0), 0, 0, 1);  
    materials.push_back(bottom);
    Material top(glm::vec3(1), 0, glm::vec3(0), 0, 0, 1);     
    materials.push_back(top);
    Material back(glm::vec3(1), 0, glm::vec3(0), 0, 0, 1);   
    materials.push_back(back); 
    Material front(glm::vec3(1), 0, glm::vec3(0), 0, 0, 1);   
    materials.push_back(front);

    createCornellBox(glm::vec3(0), glm::vec3(5), materials);

    addModel("../assets/models/dragon_recon/dragon_normals.ply", glm::vec3(0, -0.1, 0), 15, 30, 
        Material(glm::vec3(1), 0.75, glm::normalize(glm::vec3(1)), 30, glm::vec3(0.5, 1, 1), 1, 0.95, 1.25), 
        2);
}

void Scene::mirrorsEveryWhere() {
    std::vector<Material> materials;
    Material left(glm::vec3(1), 1, glm::vec3(0), 0, 0, 1);  
    materials.push_back(left);
    Material right(glm::vec3(1), 1, glm::vec3(0), 0, 0, 1);   
    materials.push_back(right);
    Material bottom(glm::vec3(1), 1, glm::vec3(0), 0, 0, 1);  
    materials.push_back(bottom);
    Material top(glm::vec3(1), 1, glm::vec3(0), 0, 0, 1);     
    materials.push_back(top);
    Material back(glm::vec3(1), 1, glm::vec3(0), 0, 0, 1);   
    materials.push_back(back); 
    Material front(glm::vec3(1), 1, glm::vec3(0), 0, 0, 1);   
    materials.push_back(front);

    createCornellBox(glm::vec3(0), glm::vec3(5), materials);

    addModel("../assets/models/dragon_recon/dragon_normals.ply", glm::vec3(0, -0.1, 0), 8, 45, 
        Material(glm::vec3(1, 0.5, 0.25), 0, glm::normalize(glm::vec3(1)), 0, glm::vec3(0.5, 1, 1), 0, 0, 1.25), 
        2);
}