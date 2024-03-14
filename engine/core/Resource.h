#pragma once

#include <string>
#include <map>
#include <memory>

#include "engine/opengl/Texture.h"
#include "engine/opengl/Shader.h"

namespace Resource
{
    std::shared_ptr<std::map<std::string, std::shared_ptr<Shader>>> getShadersMap();
    std::shared_ptr<std::map<std::string, std::shared_ptr<Texture>>> getTexturesMap();
    unsigned int &getTextureUnit();
    std::shared_ptr<Shader> getShaderProgram(std::string vertexShaderPath, std::string fragmentShaderPath);
    std::shared_ptr<Texture> getTexture(std::string textureName, bool isSpriteSheet);
}