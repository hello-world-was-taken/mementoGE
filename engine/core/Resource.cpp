#include "engine/core/Resource.h"

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

    std::shared_ptr<Shader> getShaderProgram(std::string vertexShader, std::string fragmentShader)
    {
        std::shared_ptr<std::map<std::string, std::shared_ptr<Shader>>> shaders = getShadersMap();
        std::string vertexShaderPath = "../assets/shader/" + vertexShader;
        std::string fragmentShaderPath = "../assets/shader/" + fragmentShader;
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

    std::shared_ptr<Texture> getTexture(std::string textureName, bool isSpriteSheet)
    {
        std::shared_ptr<std::map<std::string, std::shared_ptr<Texture>>> textures = getTexturesMap();
        unsigned int &textureUnit = getTextureUnit();

        std::string fullTexturePath = "../assets/texture/" + textureName;

        if (textures.get()->find(fullTexturePath) == textures.get()->end())
        {
            std::shared_ptr<Texture> texture = std::make_shared<Texture>(fullTexturePath.c_str(), textureUnit, isSpriteSheet);
            textureUnit++;
            (*textures.get())[fullTexturePath] = texture;
            return texture;
        }
        else
        {
            return (*textures.get())[fullTexturePath];
        }
    }
}