#include "Texture.h"

#include <GL/glew.h>
#include <iostream>
#include <stb_image/stb_image.h>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

Texture::Texture(const char *texturePath, unsigned int textureUnit, bool isTextureAtlas)
    : m_textureUnit(textureUnit), m_isTextureAtlas(isTextureAtlas)
{
    this->m_texturePath = texturePath;
    // stbi_set_flip_vertically_on_load(true);
    m_textureBuffer = (unsigned char *)stbi_load(texturePath, &this->m_width, &this->m_height, &this->m_nrChannels, 0);
    if (!m_textureBuffer)
    {
        throw std::runtime_error("Failed to load texture - " + m_texturePath);
    }
    m_format = (m_nrChannels == 4) ? GL_RGBA : GL_RGB;
    m_internalFormat = GL_RGBA8;

    generateTexture();

    if (m_textureBuffer)
    {
        stbi_image_free(m_textureBuffer);
    }
}

Texture::Texture(unsigned char *textureBuffer, unsigned int textureUnit, unsigned int width, unsigned int height)
    : m_textureUnit{textureUnit}, m_textureBuffer{textureBuffer}
{
    m_width = width;
    m_height = height;
    m_format = GL_RED;
    m_internalFormat = GL_R8;
    generateTexture();
}

void Texture::generateTexture()
{
    glGenTextures(1, &this->m_id);
    glActiveTexture(GL_TEXTURE0 + this->m_textureUnit);
    glBindTexture(GL_TEXTURE_2D, this->m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // for fonts
    if (m_format == GL_RED)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
    }

    // TODO: read more about this
    if (m_format == GL_RGB || m_format == GL_RED)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    glTexImage2D(GL_TEXTURE_2D,
        0,
        m_internalFormat,
        this->m_width,
        this->m_height,
        0,
        m_format,
        GL_UNSIGNED_BYTE,
        m_textureBuffer);

    // restore default alignment since it is not tied this texture only
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

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
    return m_textureUnit;
}

bool Texture::isTextureAtlas() const
{
    return m_isTextureAtlas;
}

void Texture::bind() const
{
    glActiveTexture(GL_TEXTURE0 + this->m_textureUnit);
    glBindTexture(GL_TEXTURE_2D, this->m_id);
}

void Texture::unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

int Texture::getWidth() const
{
    return m_width;
}
int Texture::getHeight() const
{
    return m_height;
}
std::string Texture::getFilePath() const
{
    return m_texturePath;
}

// TODO: Add other properties to serialize
void Texture::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "Texture";
    out << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "FilePath";
    out << YAML::Value << m_texturePath;
    out << YAML::Key << "isTextureAtlas";
    out << YAML::Value << m_isTextureAtlas;
    out << YAML::EndMap;
}

// TODO: Right now we are not using this as we need to register it to resources
void Texture::deserialize(const YAML::Node &in)
{
    std::string filePath = in["Texture"]["FilePath"].as<std::string>();
    bool isTextureAtlas = in["Texture"]["isTextureAtlas"].as<bool>();
}