#include "Shader.h"

Shader::Shader(const char* vertex_path, const char* fragment_path) {
    const char *vertex_source = parseShader(vertex_path);
    const char *fragment_source = parseShader(fragment_path);

    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_source, NULL);
    glCompileShader(vertex_shader);
    checkShaderCompileErrors(vertex_shader, "VERTEX");

    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_source, NULL);
    glCompileShader(fragment_shader);
    checkShaderCompileErrors(fragment_shader, "FRAGMENT");

    // Create shader program and attach the shaders to the program
    this->m_id = glCreateProgram();
    glAttachShader(this->m_id, vertex_shader);
    glAttachShader(this->m_id, fragment_shader);
    glLinkProgram(this->m_id);

    // Validate the program
    glValidateProgram(this->m_id);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // TODO: Am not sure if this is the right way to go about it.
    // but we need to remove the dynamically allocated memory
    delete[] vertex_source;
    delete[] fragment_source;
}


Shader::~Shader() {
    glDeleteProgram(this->m_id);
}


void Shader::use() {
    glUseProgram(this->m_id);
}


// TODO: use sstream to read the file
char* Shader::parseShader(const std::string& filePath) {
    std::ifstream stream(filePath);
    std::string line;
    std::string shader;

    if (!stream.is_open()) {
        std::cout << "Could not open the file " << filePath << std::endl;
        return NULL;
    }

    while (getline(stream, line)) {
        shader += line + "\n";
    }

    stream.close();
    // Dynamically allocate memory for the C-style string
    char* shader_cstr = new char[shader.length() + 1];
    std::strcpy(shader_cstr, shader.c_str());
    
    return shader_cstr;
};


void Shader::checkShaderCompileErrors(unsigned int shader_id, const char* shader_type) {
    int result;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);

    if (result != GL_FALSE) {
        return;
    }

    int info_log_length;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_length);

    if (info_log_length > 0) {
        char* shader_error_message = new char[info_log_length + 1];
        glGetShaderInfoLog(shader_id, info_log_length, NULL, shader_error_message);
        std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << shader_type << std::endl;
        std::cout << shader_error_message << std::endl;
        delete[] shader_error_message;
    }
};


int Shader::getUniformLocation(const std::string &name) {
    return glGetUniformLocation(this->m_id, name.c_str());
}


void Shader::setUniform1i(const std::string &name, int value) {
    glUniform1i(getUniformLocation(name), value);
}