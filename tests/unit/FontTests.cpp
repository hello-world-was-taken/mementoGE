#include "core/Font.h"

#include <gtest/gtest.h>

TEST(FontTests, PrintableAsciiMapsToAtlasIndices)
{
    Font font;

    EXPECT_EQ(font.getGlyphIndex(' '), 0);
    EXPECT_EQ(font.getGlyphIndex('?'), static_cast<std::size_t>('?' - ' '));
    EXPECT_EQ(font.getGlyphIndex('~'), 94);
}

TEST(FontTests, UnsupportedBytesFallBackToQuestionMark)
{
    Font font;
    const auto fallbackIndex = static_cast<std::size_t>('?' - ' ');

    EXPECT_EQ(font.getGlyphIndex('\n'), fallbackIndex);
    EXPECT_EQ(font.getGlyphIndex(static_cast<char>(0xE2)), fallbackIndex);
}
