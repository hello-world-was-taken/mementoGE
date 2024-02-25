#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <GL/glew.h>

class Shader {
private:
    unsigned int m_id;

    char* parseShader(const std::string& file_path);
    void checkShaderCompileErrors(unsigned int shader_id, const char* shader_type);
    int getUniformLocation(const std::string &name);
public:
    Shader(const char* vertex_path, const char* fragment_path);
    ~Shader();
    void use();
    void setUniform1i(const std::string &name, int value);
};