#include <iostream>

#include "core/Sprite.h"
#include "core/GlResourceManager.h"

Sprite::Sprite(
    std::string texturePath,
    glm::vec2 topLeft,
    float width,
    float height)
    : m_width{width},
      m_height{height}
{
    m_texture = GlResourceManager::instance().getTexture(texturePath, false);

    if (m_texture->isTextureAtlas())
    {
        glm::vec2 topLeftLocal = {topLeft.x / float(m_texture->getWidth()),
                                  topLeft.y / float(m_texture->getHeight())};

        glm::vec2 bottomLeft = {topLeft.x / float(m_texture->getWidth()),
                                (topLeft.y + height) / float(m_texture->getHeight())};

        glm::vec2 bottomRight = {(topLeft.x + width) / float(m_texture->getWidth()),
                                 (topLeft.y + height) / float(m_texture->getHeight())};

        glm::vec2 topRight = {(topLeft.x + width) / float(m_texture->getWidth()),
                              topLeft.y / float(m_texture->getHeight())};

        m_textureCoordinates = {
            topLeftLocal,
            bottomLeft,
            bottomRight,
            topRight,
        };
    }
}
Sprite::Sprite(
    std::string texturePath,
    std::vector<glm::vec2> textureCoordinates)
    : m_textureCoordinates{textureCoordinates},
      m_texture{GlResourceManager::instance().getTexture(texturePath, false)}
{
}

Sprite::Sprite()
{
}

Sprite::~Sprite()
{
}

std::vector<glm::vec2> Sprite::getTextureCoordinates() const
{
    std::vector<glm::vec2> coords = m_textureCoordinates;

    if (m_flipX)
    {
        std::swap(coords[0], coords[3]); // topLeft <-> topRight
        std::swap(coords[1], coords[2]); // bottomLeft <-> bottomRight
    }

    if (m_flipY)
    {
        std::swap(coords[0], coords[1]); // topLeft <-> bottomLeft
        std::swap(coords[3], coords[2]); // topRight <-> bottomRight
    }

    return coords;
}

std::shared_ptr<Texture> Sprite::getTexture()
{
    return m_texture;
}

std::string Sprite::getTexturePath()
{
    return m_texture->getFilePath();
}

glm::vec4
Sprite::getColor()
{
    return m_color;
}

void Sprite::setFlipX(bool flip)
{
    m_flipX = flip;
}

void Sprite::setFlipY(bool flip)
{
    m_flipY = flip;
}

bool Sprite::isFlippedX() const
{
    return m_flipX;
}

bool Sprite::isFlippedY() const
{
    return m_flipY;
}

float Sprite::getWidth() const
{
    return m_width;
};
float Sprite::getHeight() const
{
    return m_height;
};

void Sprite::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "Sprite";
    out << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "textureCoordinates";
    out << YAML::Value << YAML::BeginSeq;
    for (auto &coord : m_textureCoordinates)
    {
        out << YAML::BeginSeq;
        out << coord.x;
        out << coord.y;
        out << YAML::EndSeq;
    }
    out << YAML::EndSeq;

    // TODO: some sprites might not have a texture only color and vise versa
    out << YAML::Key << "Color";
    out << YAML::Value << YAML::BeginSeq;
    out << m_color.r;
    out << m_color.g;
    out << m_color.b;
    out << m_color.a;
    out << YAML::EndSeq;

    out << YAML::Key << "FlipX" << YAML::Value << m_flipX;
    out << YAML::Key << "FlipY" << YAML::Value << m_flipY;

    m_texture->serialize(out);
    out << YAML::EndMap;
}

void Sprite::deserialize(const YAML::Node &in)
{
    m_textureCoordinates.clear();
    auto textureCoordinates = in["Sprite"]["textureCoordinates"];
    for (std::size_t i = 0; i < textureCoordinates.size(); i++)
    {
        m_textureCoordinates.push_back({textureCoordinates[i][0].as<float>(), textureCoordinates[i][1].as<float>()});
    }

    auto color = in["Sprite"]["Color"];
    m_color = {color[0].as<float>(), color[1].as<float>(), color[2].as<float>(), color[3].as<float>()};

    auto texture = in["Sprite"]["Texture"];
    std::string filePath = texture["FilePath"].as<std::string>();
    bool isTextureAtlas = texture["isTextureAtlas"].as<bool>();
    m_texture = GlResourceManager::instance().getTexture(filePath, isTextureAtlas);
    m_texture.get()->bind();

    m_flipX = in["Sprite"]["FlipX"] ? in["Sprite"]["FlipX"].as<bool>() : false;
    m_flipY = in["Sprite"]["FlipY"] ? in["Sprite"]["FlipY"].as<bool>() : false;
}