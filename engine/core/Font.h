#pragma once

#include "opengl/Texture.h"

#include <memory>
#include <stb_truetype/stb_truetype.h>

struct Font
{
    std::string fontPath;
    std::shared_ptr<Texture> atlasTexture;
    
    std::array<stbtt_packedchar, 95> packedChars;
    std::array<stbtt_aligned_quad, 95> alignedQuad;

    // These values are hard coded in GlResource::getFontTexture(). Keep in sync
    float pixelSize = 64.0f;
    unsigned int firstChar = 32; // space
    unsigned int charCount = 95; // up to DEL

    const stbtt_packedchar &getPackedChar(char c) const
    {
        return packedChars[c - firstChar];
    }

    const stbtt_aligned_quad &getAlignedQuad(char c) const
    {
        return alignedQuad[c - firstChar];
    }
};