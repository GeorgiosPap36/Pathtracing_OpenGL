#ifndef COMPUTE_SHADER_H
#define COMPUTE_SHADER_H

#include "../dependencies/glad.h"
#include "glm/glm.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class ComputeShader {
public:
    unsigned int ID;

    ComputeShader(const char* computeShaderPath);

    void use();
    void dispatch(GLuint num_groups_x​, GLuint num_groups_y​, GLuint num_groups_z​);
    void setInt(const std::string& name, int value) const;
    void setBool(const std::string& name, bool value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;

private:
    void checkCompileErrors(GLuint shader, std::string type);
};

#endif