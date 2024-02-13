#include <iostream>
#include <string>
#include <fstream>
#include <GL/glew.h>


char* parseShader(std::string& filePath) {
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

void checkShaderCompilationStatus(unsigned int shader_id) {
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
        std::cout << shader_error_message << std::endl;
        delete[] shader_error_message;
    }
};

unsigned int createShaderProgram() {
    GLuint program_id = glCreateProgram();

    // Create a vertex shader
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    // since cwd is the build directory, we need to go one level up to find the shader file
    std::string vertex_shader_file_path = "../opengl/shader/vertex.shader";
    char* vertex_shader_source = parseShader(vertex_shader_file_path);

    glShaderSource(vertex_shader_id, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader_id);

    // Check the vertex shader compilation status
    checkShaderCompilationStatus(vertex_shader_id);

    // Create a fragment shader
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    std::string fragment_shader_file_path = "../opengl/shader/fragment.shader";
    char* fragment_shader_source = parseShader(fragment_shader_file_path);
    glShaderSource(fragment_shader_id, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader_id);

    // Check the fragment shader compilation status
    checkShaderCompilationStatus(fragment_shader_id);

    // Attach the shaders to the program
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);

    // Validate the program
    glValidateProgram(program_id);

    // TODO: Am not sure if this is the right way to go about it.
    delete[] vertex_shader_source;
    delete[] fragment_shader_source;

    return program_id;
};