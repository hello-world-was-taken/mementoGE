#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <memory>
#include <yaml-cpp/yaml.h>

#include "opengl/Texture.h"

class Sprite
{
public:
    // TODO: We don't need the two constructors. The first one is just a specialized version
    // of the second one. We can get away with default values.
    Sprite(
        std::string texturePath, bool isTextureAtlas);
    Sprite(
        std::string texturePath,
        bool isTextureAtlas,
        unsigned int subTextureSpanX,
        unsigned int subTextureSpanY,
        unsigned int subTextureSize,
        unsigned int subTextureIndexX,
        unsigned int subTextureIndexY);
    Sprite(
        std::string texturePath,
        bool isTextureAtlas,
        std::vector<glm::vec2> textureCoordinates);
    Sprite(); // TODO: Should only be used for serialization
    ~Sprite();

    glm::vec4 getColor();

    std::vector<glm::vec2> getTextureCoordinates();
    std::shared_ptr<Texture> getTexture();
    std::string getTexturePath();

    void setFlipX(bool flip);
    void setFlipY(bool flip);

    bool isFlippedX() const;
    bool isFlippedY() const;

    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);

private:
    std::shared_ptr<Texture> m_texture;
    std::vector<glm::vec2> m_textureCoordinates = {
        {0.0f, 1.0f}, // top left
        {0.0f, 0.0f}, // bottom left
        {1.0f, 0.0f}, // bottom right
        {1.0f, 1.0f}, // top right
    };

    glm::vec4 m_color = {1.0f, 1.0f, 1.0f, 1.0f}; // white

    bool m_flipX = false;
    bool m_flipY = false;

    // TODO: a sprite could just be a color as well
};
