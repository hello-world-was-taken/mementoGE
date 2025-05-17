#include <yaml-cpp/yaml.h>
#include <stb_image/stb_image.h>
#include <GL/glew.h>
#include <iostream>

#include "Texture.h"

Texture::Texture(const char *texture_path, int texture_unit, bool isTextureAtlas)
    : m_texture_unit(texture_unit),
      m_is_texture_atlas(isTextureAtlas)
{
    this->m_texture_path = texture_path;
    stbi_set_flip_vertically_on_load(true);
    m_texture_buffer = (char *)stbi_load(texture_path, &this->m_width, &this->m_height, &this->m_nrChannels, 0);
    if (!m_texture_buffer)
    {
        std::cout << "Failed to load texture" << std::endl;
        return;
    }

    glGenTextures(1, &this->m_id);
    glActiveTexture(GL_TEXTURE0 + this->m_texture_unit);
    glBindTexture(GL_TEXTURE_2D, this->m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->m_width, this->m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_texture_buffer);

    if (m_texture_buffer)
    {
        stbi_image_free(m_texture_buffer);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture()
{
    std::cout << "Texture destructor called" << std::endl;
    glDeleteTextures(1, &this->m_id);
}

unsigned int Texture::getTextureId() const
{
    return m_id;
}

unsigned int Texture::getTextureUnit() const
{
    return m_texture_unit;
}

bool Texture::isTextureAtlas() const
{
    return m_is_texture_atlas;
}

void Texture::bind() const
{
    glActiveTexture(GL_TEXTURE0 + this->m_texture_unit);
    glBindTexture(GL_TEXTURE_2D, this->m_id);
}

void Texture::unbind() const
{
    // glBindTexture(GL_TEXTURE_2D, 0);
}

// TODO: Add other properties to serialize
void Texture::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "Texture";
    out << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "FilePath";
    out << YAML::Value << m_texture_path;
    out << YAML::Key << "isTextureAtlas";
    out << YAML::Value << m_is_texture_atlas;
    out << YAML::EndMap;
}

// TODO: Right now we are not using this as we need to register it to resources
void Texture::deserialize(const YAML::Node &in)
{
    std::string filePath = in["Texture"]["FilePath"].as<std::string>();
    bool isTextureAtlas = in["Texture"]["isTextureAtlas"].as<bool>();
}