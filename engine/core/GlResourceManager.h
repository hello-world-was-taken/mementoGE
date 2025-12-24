#pragma once

#include "opengl/Shader.h"
#include "opengl/Texture.h"

#include <map>
#include <memory>
#include <stb_truetype/stb_truetype.h>

class GlResourceManager
{
public:
    static GlResourceManager &instance();

    // ASCII 32(Space) to ASCII 126(~)
    static const unsigned int codePointOfFirstChar = 32;
    static const unsigned int charsToIncludeInFontAtlas = 95;

    std::shared_ptr<Shader> getShaderProgram(const std::string &vertexPath, const std::string &fragmentPath);
    std::shared_ptr<Texture> getTexture(const std::string &texturePath, bool isSpriteSheet);
    std::shared_ptr<Texture> getFontTexture(const std::string &fontPath);
    std::array<stbtt_packedchar, charsToIncludeInFontAtlas> &getFontPackedChar(const std::string &fontPath);
    std::array<stbtt_aligned_quad, charsToIncludeInFontAtlas> &getFontAlignedQuad(const std::string &fontPath);

private:
    GlResourceManager() = default;
    ~GlResourceManager() = default;

    // Disable copy/move
    GlResourceManager(const GlResourceManager &) = delete;
    GlResourceManager &operator=(const GlResourceManager &) = delete;
    GlResourceManager(GlResourceManager &&) = delete;
    GlResourceManager &operator=(GlResourceManager &&) = delete;

    unsigned int m_textureUnit = 1;

    // Font pixel height
    float fontSize = 64.0f;
    unsigned int fontAtlasWidth = 1024;
    unsigned int fontAtlasHeight = 1024;

    std::map<std::string, std::shared_ptr<Shader>> shaders;
    std::map<std::string, std::shared_ptr<Texture>> textureByPath;
    std::map<std::string, std::array<stbtt_packedchar, charsToIncludeInFontAtlas>> packedCharByPath;
    std::map<std::string, std::array<stbtt_aligned_quad, charsToIncludeInFontAtlas>> alignedQuadByPath;
};