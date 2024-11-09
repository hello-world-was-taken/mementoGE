#pragma once

#include <iostream>
#include <GL/glew.h>
#include <stb_image/stb_image.h>
#include <yaml-cpp/yaml.h>

class Texture
{
private:
    unsigned int m_id;
    int m_nrChannels = 4;  // 4 for png, 3 for jpg
    int m_width = 0;
    int m_height = 0;
    int m_texture_unit = 0; // The texture slot in the shader. Default is 0 meaning use the color set.
    char* m_texture_buffer = nullptr;
    bool m_is_texture_atlas = false;
    std::string m_texture_path;
public:
    Texture(
        const char *texture_path,
        int texture_unit,
        bool is_texture_atlas = false);
    Texture();  // TODO: should only be used for serialization
    ~Texture();
    unsigned int getId() const;
    unsigned int getTextureUnit() const;
    bool isTextureAtlas() const;
    void bind() const;
    void unbind() const;
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    std::string getFilePath() const { return m_texture_path; }

    // TODO: Should I even have this here? Since it is an abstraction of OpenGL.
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);
};