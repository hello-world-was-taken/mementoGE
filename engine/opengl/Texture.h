#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

class Texture
{
public:
    Texture(const char *texturePath, bool is_texture_atlas = false);
    // constructor used to load font
    Texture(unsigned char *textureBuffer, unsigned int width, unsigned int height);
    Texture(); // TODO: should only be used for serialization
    ~Texture();
    unsigned int getTextureId() const;
    bool isTextureAtlas() const;
    void bind(unsigned int textureUnit = 0) const;
    void unbind() const;

    int getWidth() const;
    int getHeight() const;
    std::string getFilePath() const;

    glm::vec3 getColorAtPixel(int x, int y) const;

    // TODO: Should I even have this here? Since it is an abstraction of OpenGL.
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);

private:
    void generateTexture();

private:
    unsigned int m_id;
    int m_nrChannels = 4; // 4 for png, 3 for jpg
    int m_width = 0;
    int m_height = 0;
    unsigned char *m_textureBuffer = nullptr;
    bool m_isTextureAtlas = false;
    std::string m_texturePath; // could be null when texture is font since we do the loading and rasterization in memory
    unsigned int m_format;     // image format from the cpu side
    unsigned int m_internalFormat; // image format when storing on the GPU
};
