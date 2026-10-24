#define STB_TRUETYPE_IMPLEMENTATION

#include "core/GlResourceManager.h"

#include "opengl/Shader.h"
#include "opengl/Texture.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

GlResourceManager &GlResourceManager::instance()
{
    static GlResourceManager rm;
    return rm;
}

void GlResourceManager::clear()
{
    shaders.clear();
    textureByPath.clear();
    packedCharByPath.clear();
    alignedQuadByPath.clear();
    fontMetricsByPath.clear();
}

std::shared_ptr<Shader> GlResourceManager::getShaderProgram(
    const std::string &vertexShaderPath, const std::string &fragmentShaderPath)
{
    std::string fullShaderPath = vertexShaderPath + fragmentShaderPath;

    auto it = shaders.find(fullShaderPath);
    if (it == shaders.end())
    {
        auto shader = std::make_shared<Shader>(vertexShaderPath.c_str(), fragmentShaderPath.c_str());
        shaders[fullShaderPath] = shader;
        return shader;
    }

    return it->second;
}

std::shared_ptr<Texture> GlResourceManager::getTexture(const std::string &texturePath)
{
    auto it = textureByPath.find(texturePath);
    if (it == textureByPath.end())
    {
        auto texture = std::make_shared<Texture>(texturePath.c_str());
        textureByPath[texturePath] = texture;
        return texture;
    }

    return it->second;
}

std::shared_ptr<Texture> GlResourceManager::getFontTexture(const std::string &fontPath)
{
    auto it = textureByPath.find(fontPath);
    if (it != textureByPath.end())
    {
        return it->second;
    }

    std::ifstream fontFileStream{fontPath.c_str(), std::ios::binary};

    if (!fontFileStream)
    {
        std::cerr << "Failed to load font:" << fontPath << std::endl;
        return nullptr;
    }

    size_t size = std::filesystem::file_size(fontPath);

    std::unique_ptr<uint8_t[]> fontDataBuf = std::make_unique<uint8_t[]>(size);
    fontFileStream.read(reinterpret_cast<char *>(fontDataBuf.get()), size);

    std::unique_ptr<uint8_t[]> fontAtlasBitmap = std::make_unique<uint8_t[]>(fontAtlasWidth * fontAtlasHeight);
    memset(fontAtlasBitmap.get(), 0, fontAtlasWidth * fontAtlasHeight);

    std::array<stbtt_packedchar, charsToIncludeInFontAtlas> packedChars;
    std::array<stbtt_aligned_quad, charsToIncludeInFontAtlas> alignedQuads;
    FontMetrics metrics;

    stbtt_fontinfo fontInfo;
    const int fontOffset = stbtt_GetFontOffsetForIndex(fontDataBuf.get(), 0);
    if (fontOffset >= 0 && stbtt_InitFont(&fontInfo, fontDataBuf.get(), fontOffset))
    {
        int ascent;
        int descent;
        int lineGap;
        stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

        const float metricScale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);
        metrics.ascent = static_cast<float>(ascent) * metricScale;
        metrics.descent = static_cast<float>(descent) * metricScale;
        metrics.lineGap = static_cast<float>(lineGap) * metricScale;

        for (unsigned int left = 0; left < charsToIncludeInFontAtlas; ++left)
        {
            for (unsigned int right = 0; right < charsToIncludeInFontAtlas; ++right)
            {
                const int leftCodepoint = static_cast<int>(codePointOfFirstChar + left);
                const int rightCodepoint = static_cast<int>(codePointOfFirstChar + right);
                metrics.kerning[left * charsToIncludeInFontAtlas + right] =
                    static_cast<float>(stbtt_GetCodepointKernAdvance(&fontInfo, leftCodepoint, rightCodepoint)) *
                    metricScale;
            }
        }

        metrics.valid = true;
    }

    stbtt_pack_context ctx;

    stbtt_PackBegin(&ctx,      // stbtt_pack_context (this call will initialize it)
        fontAtlasBitmap.get(), // Font Atlas bitmap data
        fontAtlasWidth,
        fontAtlasHeight,
        0, // Stride in bytes
        1, // Padding between the glyphs
        nullptr);

    stbtt_PackFontRange(&ctx,      // stbtt_pack_context
        fontDataBuf.get(),         // Font Atlas texture data
        0,                         // Font Index
        fontSize,                  // Size of font in pixels. (Use STBTT_POINT_SIZE(fontSize) to use points)
        codePointOfFirstChar,      // Code point of the first character
        charsToIncludeInFontAtlas, // No. of charecters to be included in the font atlas
        packedChars.begin()        // stbtt_packedchar array, this struct will contain the data to render a glyph
    );
    stbtt_PackEnd(&ctx);

    for (int i = 0; i < charsToIncludeInFontAtlas; i++)
    {
        float unusedX = 0.0f;
        float unusedY = 0.0f;

        stbtt_GetPackedQuad(packedChars.begin(), // Array of stbtt_packedchar
            fontAtlasWidth,                      // Width of the font atlas texture
            fontAtlasHeight,                     // Height of the font atlas texture
            i,                                   // Index of the glyph
            &unusedX,
            &unusedY,         // current position of the glyph in screen pixel coordinates, (not required as we have a
                              // different corrdinate system)
            &alignedQuads[i], // stbtt_alligned_quad struct. (this struct mainly consists of the texture
                              // coordinates)
            0 // Allign X and Y position to a integer (doesn't matter because we are not using 'unusedX' and
              // 'unusedY')
        );
    }

    packedCharByPath[fontPath] = std::move(packedChars);
    alignedQuadByPath[fontPath] = std::move(alignedQuads);
    fontMetricsByPath[fontPath] = std::move(metrics);

    // create font texture
    auto texture = std::make_shared<Texture>(fontAtlasBitmap.get(), fontAtlasWidth, fontAtlasHeight);
    textureByPath[fontPath] = texture;
    return texture;
}

void GlResourceManager::forgetTexture(const std::string &texturePath)
{
    // This only evicts the resource manager's cached references. If another
    // shared_ptr still owns the Texture, it remains alive. The OpenGL texture is
    // deleted by Texture::~Texture() only after the final owner releases it.
    textureByPath.erase(texturePath);
    packedCharByPath.erase(texturePath);
    alignedQuadByPath.erase(texturePath);
    fontMetricsByPath.erase(texturePath);
}

std::array<stbtt_packedchar, GlResourceManager::charsToIncludeInFontAtlas> &GlResourceManager::getFontPackedChar(
    const std::string &fontPath)
{
    auto it = packedCharByPath.find(fontPath);
    if (it == packedCharByPath.end())
    {
        throw std::runtime_error("Font is not rasterized! Load the font before trying to access packed char.");
    }

    return it->second;
}

std::array<stbtt_aligned_quad, GlResourceManager::charsToIncludeInFontAtlas> &GlResourceManager::getFontAlignedQuad(
    const std::string &fontPath)
{
    auto it = alignedQuadByPath.find(fontPath);
    if (it == alignedQuadByPath.end())
    {
        throw std::runtime_error("Font is not rasterized! Load the font before trying to access aligned quad.");
    }

    return it->second;
}

const FontMetrics &GlResourceManager::getFontMetrics(const std::string &fontPath)
{
    auto it = fontMetricsByPath.find(fontPath);
    if (it == fontMetricsByPath.end())
    {
        throw std::runtime_error("Font is not rasterized! Load the font before trying to access its metrics.");
    }

    return it->second;
}
