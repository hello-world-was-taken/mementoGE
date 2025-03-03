#include "core/Resource.h"

namespace Resource
{
    std::shared_ptr<std::map<std::string, std::shared_ptr<Shader>>> getShadersMap()
    {
        static std::shared_ptr<std::map<std::string, std::shared_ptr<Shader>>> shadersMap =
            std::make_shared<std::map<std::string, std::shared_ptr<Shader>>>();
        return shadersMap;
    }

    std::shared_ptr<std::map<std::string, std::shared_ptr<Texture>>> getTexturesMap()
    {
        static std::shared_ptr<std::map<std::string, std::shared_ptr<Texture>>> texturesMap =
            std::make_shared<std::map<std::string, std::shared_ptr<Texture>>>();
        return texturesMap;
    }

    unsigned int &getTextureUnit()
    {
        static unsigned int textureUnit = 1;
        return textureUnit;
    }

    std::shared_ptr<Shader> getShaderProgram(std::string vertexShaderPath, std::string fragmentShaderPath)
    {
        std::shared_ptr<std::map<std::string, std::shared_ptr<Shader>>> shaders = getShadersMap();
        std::string fullShaderPath = vertexShaderPath + fragmentShaderPath;

        if (shaders.get()->find(fullShaderPath) == shaders.get()->end())
        {
            std::shared_ptr<Shader> shader = std::make_shared<Shader>(vertexShaderPath.c_str(), fragmentShaderPath.c_str());
            (*shaders.get())[fullShaderPath] = shader;
            return shader;
        }
        else
        {
            return (*shaders.get())[fullShaderPath];
        }
    }

    std::shared_ptr<Texture> getTexture(std::string texturePath, bool isSpriteSheet)
    {
        std::shared_ptr<std::map<std::string, std::shared_ptr<Texture>>> textures = getTexturesMap();
        unsigned int &textureUnit = getTextureUnit();

        if (textures.get()->find(texturePath) == textures.get()->end())
        {
            std::shared_ptr<Texture> texture = std::make_shared<Texture>(texturePath.c_str(), textureUnit, isSpriteSheet);
            textureUnit++;
            (*textures.get())[texturePath] = texture;
            return texture;
        }
        else
        {
            return (*textures.get())[texturePath];
        }
    }
}