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
    Sprite(
        std::string texturePath,
        std::vector<glm::vec2> textureCoordinates);
    Sprite(
        std::string texturePath,
        glm::vec2 topLeft,
        float width,
        float height);

    Sprite(); // TODO: Should only be used for serialization and Frame creation
    ~Sprite();

    glm::vec4 getColor();

    std::vector<glm::vec2> getTextureCoordinates() const;
    std::shared_ptr<Texture> getTexture();
    std::string getTexturePath();

    void setFlipX(bool flip);
    void setFlipY(bool flip);

    bool isFlippedX() const;
    bool isFlippedY() const;

    float getWidth() const;
    float getHeight() const;

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

    float m_width;
    float m_height;

    // TODO: a sprite could just be a color as well
};
