#include <iostream>

#include "core/Sprite.h"
#include "core/Resource.h"

Sprite::Sprite(
    std::string&& texturePath)
{
    m_texture = Resource::getTexture(texturePath, false);
}

// subTextureSpanX: The span of the subtexture in the x direction.
// subTextureSpanY: The span of the subtexture in the y direction.
// subTextureSize: The size of the subtexture. Assumed to be a square.
// subTextureIndexX: The index of the subtexture in the x direction where the origin is the bottom left corner.
// subTextureIndexY: The index of the subtexture in the y direction where the origin is the bottom left corner.
Sprite::Sprite(
    std::shared_ptr<Texture> texture,
    unsigned int subTextureSpanX,
    unsigned int subTextureSpanY,
    unsigned int subTextureSize,
    unsigned int subTextureIndexX,
    unsigned int subTextureIndexY)
{
    m_texture = texture;

    if (texture->isTextureAtlas())
    {
        glm::vec2 topLeft = {float(subTextureSize * subTextureIndexX) / float(texture->getWidth()),
                             float((subTextureSize * (subTextureIndexY + subTextureSpanY))) / float(texture->getHeight())};

        glm::vec2 bottomLeft = {float(subTextureSize * subTextureIndexX) / float(texture->getWidth()),
                                float(subTextureSize * subTextureIndexY) / float(texture->getHeight())};

        glm::vec2 bottomRight = {float(subTextureSize * (subTextureIndexX + subTextureSpanX)) / float(texture->getWidth()),
                                 float(subTextureSize * (subTextureIndexY)) / float(texture->getHeight())};

        glm::vec2 topRight = {float(subTextureSize * (subTextureIndexX + subTextureSpanX)) / float(texture->getWidth()),
                              float(subTextureSize * (subTextureIndexY + subTextureSpanY)) / float(texture->getHeight())};

        m_textureCoordinates = {
            topLeft,
            bottomLeft,
            bottomRight,
            topRight,
        };
    }
}

Sprite::Sprite()
{
}

Sprite::~Sprite()
{
}

std::vector<glm::vec2> Sprite::getTextureCoordinates()
{
    return m_textureCoordinates;
}

std::shared_ptr<Texture> Sprite::getTexture()
{
    return m_texture;
}

glm::vec4 Sprite::getColor()
{
    return m_color;
}

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
    std::cout << "deserializing transform" << std::endl;

    auto color = in["Sprite"]["Color"];
    m_color = {color[0].as<float>(), color[1].as<float>(), color[2].as<float>(), color[3].as<float>()};

    auto texture = in["Sprite"]["Texture"];
    std::string filePath = texture["FilePath"].as<std::string>();
    bool isTextureAtlas = texture["isTextureAtlas"].as<bool>();
    m_texture = Resource::getTexture(filePath, isTextureAtlas);
    m_texture.get()->bind();
}