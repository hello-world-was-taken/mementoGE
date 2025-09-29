#pragma once

#include "opengl/Shader.h"
#include "opengl/Texture.h"

#include <memory>
#include <map>

class ResourceManager
{
public:
    static ResourceManager &instance();

    std::shared_ptr<Shader> getShaderProgram(const std::string &vertexPath, const std::string &fragmentPath);
    std::shared_ptr<Texture> getTexture(const std::string &texturePath, bool isSpriteSheet);

private:
    ResourceManager() = default;
    ~ResourceManager() = default;

    // Disable copy/move
    ResourceManager(const ResourceManager &) = delete;
    ResourceManager &operator=(const ResourceManager &) = delete;
    ResourceManager(ResourceManager &&) = delete;
    ResourceManager &operator=(ResourceManager &&) = delete;

    std::map<std::string, std::shared_ptr<Shader>> shaders;
    std::map<std::string, std::shared_ptr<Texture>> textures;
    unsigned int textureUnit = 1;
};