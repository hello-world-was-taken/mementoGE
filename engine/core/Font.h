#pragma once

#include "opengl/Texture.h"

#include <array>
#include <cstddef>
#include <memory>
#include <stb_truetype/stb_truetype.h>
#include <string>

struct FontMetrics
{
    static constexpr std::size_t glyphCount = 95;

    // Vertical values and kerning pairs are stored in atlas-pixel units so
    // they can be scaled alongside stbtt_packedchar's offsets and advances.
    float ascent = 0.0f;
    float descent = 0.0f;
    float lineGap = 0.0f;
    std::array<float, glyphCount * glyphCount> kerning{};
    bool valid = false;
};

struct Font
{
    static constexpr std::size_t glyphCount = FontMetrics::glyphCount;

    std::string fontPath;
    std::shared_ptr<Texture> atlasTexture;
    
    std::array<stbtt_packedchar, glyphCount> packedChars;
    std::array<stbtt_aligned_quad, glyphCount> alignedQuad;

    // These values are hard coded in GlResource::getFontTexture(). Keep in sync
    float pixelSize = 64.0f;
    unsigned int firstChar = 32; // space
    unsigned int charCount = glyphCount; // printable ASCII, up to '~'
    FontMetrics metrics;

    std::size_t getGlyphIndex(char c) const
    {
        const auto character = static_cast<unsigned int>(static_cast<unsigned char>(c));
        if (character < firstChar || character >= firstChar + charCount)
        {
            // The atlas currently contains printable ASCII only. Falling back
            // keeps UTF-8 bytes and control characters from indexing outside
            // the glyph arrays.
            return static_cast<std::size_t>('?' - firstChar);
        }

        return static_cast<std::size_t>(character - firstChar);
    }

    const stbtt_packedchar &getPackedChar(char c) const
    {
        return packedChars[getGlyphIndex(c)];
    }

    const stbtt_aligned_quad &getAlignedQuad(char c) const
    {
        return alignedQuad[getGlyphIndex(c)];
    }

    float getKerning(char left, char right) const
    {
        if (!metrics.valid)
        {
            return 0.0f;
        }

        return metrics.kerning[getGlyphIndex(left) * glyphCount + getGlyphIndex(right)];
    }
};
