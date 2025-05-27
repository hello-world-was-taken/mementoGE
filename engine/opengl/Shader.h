#pragma once

#include <string>
#include <glm/glm.hpp>

class Shader
{
public:
    Shader(const char *vertex_path, const char *fragment_path);
    ~Shader();
    void use();

    void setUniform1i(const std::string &name, int value);
    void setUniform1f(const std::string &name, float value);
    void setUniform4fv(const std::string &name, glm::mat4 value);
    void setUniform4f(const std::string &name, glm::vec4 value);
    void setMultipleTextureUnits(const std::string &name, int *texture_units, int size);

private:
    unsigned int m_id;

    char *parseShader(const std::string &file_path);
    void checkShaderCompileErrors(unsigned int shader_id, const char *shader_type);
    int getUniformLocation(const std::string &name);
};