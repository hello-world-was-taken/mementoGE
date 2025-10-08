#pragma once

#include "opengl/Shader.h"
#include "opengl/Texture.h"

#include <memory>
#include <map>

class GlResourceManager
{
public:
    static GlResourceManager &instance();

    std::shared_ptr<Shader> getShaderProgram(const std::string &vertexPath, const std::string &fragmentPath);
    std::shared_ptr<Texture> getTexture(const std::string &texturePath, bool isSpriteSheet);

private:
    GlResourceManager() = default;
    ~GlResourceManager() = default;

    // Disable copy/move
    GlResourceManager(const GlResourceManager &) = delete;
    GlResourceManager &operator=(const GlResourceManager &) = delete;
    GlResourceManager(GlResourceManager &&) = delete;
    GlResourceManager &operator=(GlResourceManager &&) = delete;

    std::map<std::string, std::shared_ptr<Shader>> shaders;
    std::map<std::string, std::shared_ptr<Texture>> textures;
    unsigned int textureUnit = 1;
};