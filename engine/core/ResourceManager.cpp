#include "core/ResourceManager.h"

#include "opengl/Shader.h"
#include "opengl/Texture.h"

#include <memory>
#include <stdexcept>

ResourceManager &ResourceManager::instance()
{
    static ResourceManager rm;
    return rm;
}

std::shared_ptr<Shader> ResourceManager::getShaderProgram(const std::string &vertexShaderPath,
                                                          const std::string &fragmentShaderPath)
{
    std::string fullShaderPath = vertexShaderPath + fragmentShaderPath;

    auto it = shaders.find(fullShaderPath);
    if (it == shaders.end())
    {
        auto shader = std::make_shared<Shader>(vertexShaderPath.c_str(),
                                               fragmentShaderPath.c_str());
        shaders[fullShaderPath] = shader;
        return shader;
    }

    return it->second;
}

// TODO: can we remove the isSpriteSheet bool here.
// we have two d/t classes now. SpriteSheet.h and just Sprite.h
std::shared_ptr<Texture> ResourceManager::getTexture(const std::string &texturePath,
                                                     bool isSpriteSheet)
{
    if (textureUnit == 16)
    {
        // we use textureUnit 16 for framebuffer texture
        throw std::runtime_error("mementoGE: Max texture limit reached!");
    }

    auto it = textures.find(texturePath);
    if (it == textures.end())
    {
        auto texture = std::make_shared<Texture>(texturePath.c_str(),
                                                 textureUnit,
                                                 isSpriteSheet);
        textureUnit++;
        textures[texturePath] = texture;

        texture->bind(); // auto-bind on load
        return texture;
    }

    return it->second;
}