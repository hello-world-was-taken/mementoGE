#include "core/components/Text.h"

#include <gtest/gtest.h>

#include <memory>

namespace
{
std::shared_ptr<Font> makeTestFont()
{
    auto font = std::make_shared<Font>();
    font->pixelSize = 10.0f;
    font->metrics.ascent = 8.0f;
    font->metrics.descent = -2.0f;
    font->metrics.lineGap = 2.0f;
    font->metrics.valid = true;

    auto setGlyph = [&](char character, float width, float advance)
    {
        auto &packed = font->packedChars[font->getGlyphIndex(character)];
        packed.x0 = 1;
        packed.y0 = 1;
        packed.x1 = static_cast<unsigned short>(1.0f + width);
        packed.y1 = 11;
        packed.xoff = 0.0f;
        packed.yoff = -8.0f;
        packed.xoff2 = width;
        packed.yoff2 = 2.0f;
        packed.xadvance = advance;

        auto &quad = font->alignedQuad[font->getGlyphIndex(character)];
        quad.s0 = 0.0f;
        quad.t0 = 0.0f;
        quad.s1 = 1.0f;
        quad.t1 = 1.0f;
    };

    setGlyph('A', 6.0f, 7.0f);
    setGlyph('V', 6.0f, 7.0f);

    auto &space = font->packedChars[font->getGlyphIndex(' ')];
    space.xadvance = 4.0f;

    font->metrics.kerning[font->getGlyphIndex('A') * Font::glyphCount + font->getGlyphIndex('V')] = -1.0f;
    return font;
}
} // namespace

TEST(TextTests, UsesAdvanceKerningAndLetterSpacing)
{
    Text text;
    text.content = "AV";
    text.font = makeTestFont();
    text.fontSize = 10.0f;
    text.letterSpacing = 2.0f;

    EXPECT_FLOAT_EQ(text.measure().x, 15.0f);
    EXPECT_FLOAT_EQ(text.measure().y, 10.0f);

    Transform transform{};
    text.rebuild(transform);

    ASSERT_EQ(text.vertices.size(), 8u);
    EXPECT_FLOAT_EQ(text.vertices[0].position.x, 6.0f);
    EXPECT_FLOAT_EQ(text.vertices[1].position.x, 0.0f);
    EXPECT_FLOAT_EQ(text.vertices[4].position.x, 14.0f);
    EXPECT_FLOAT_EQ(text.vertices[5].position.x, 8.0f);
}

TEST(TextTests, AlignsGlyphsToTheFontBaseline)
{
    Text text;
    text.content = "A";
    text.font = makeTestFont();
    text.fontSize = 10.0f;

    Transform transform{};
    transform.position = {3.0f, 5.0f, 2.0f};
    text.rebuild(transform);

    ASSERT_EQ(text.vertices.size(), 4u);
    EXPECT_FLOAT_EQ(text.vertices[0].position.y, 15.0f);
    EXPECT_FLOAT_EQ(text.vertices[2].position.y, 5.0f);
    EXPECT_FLOAT_EQ(text.vertices[0].position.z, 2.0f);
}

TEST(TextTests, MeasuresAndBuildsMultipleLinesWithLineSpacing)
{
    Text text;
    text.content = "A\nA";
    text.font = makeTestFont();
    text.fontSize = 10.0f;
    text.lineSpacing = 3.0f;

    const glm::vec2 size = text.measure();
    EXPECT_FLOAT_EQ(size.x, 7.0f);
    EXPECT_FLOAT_EQ(size.y, 25.0f);

    Transform transform{};
    text.rebuild(transform);

    ASSERT_EQ(text.vertices.size(), 8u);
    EXPECT_FLOAT_EQ(text.vertices[0].position.y, 25.0f);
    EXPECT_FLOAT_EQ(text.vertices[2].position.y, 15.0f);
    EXPECT_FLOAT_EQ(text.vertices[4].position.y, 10.0f);
    EXPECT_FLOAT_EQ(text.vertices[6].position.y, 0.0f);
}

TEST(TextTests, SpacesAdvanceWithoutCreatingDegenerateGeometry)
{
    Text text;
    text.content = "A A";
    text.font = makeTestFont();
    text.fontSize = 10.0f;

    EXPECT_FLOAT_EQ(text.measure().x, 18.0f);

    Transform transform{};
    text.rebuild(transform);
    EXPECT_EQ(text.vertices.size(), 8u);
}

#ifdef EDITOR_BUILD
TEST(TextTests, RoundTripsCharacterAndLineSpacing)
{
    Text text;
    text.content = "First\nSecond";
    text.fontSize = 42.0f;
    text.letterSpacing = 1.5f;
    text.lineSpacing = 6.0f;

    YAML::Emitter out;
    out << YAML::BeginMap;
    text.serialize(out);
    out << YAML::EndMap;

    Text copy;
    copy.deserialize(YAML::Load(out.c_str()));

    EXPECT_EQ(copy.content, text.content);
    EXPECT_FLOAT_EQ(copy.fontSize, text.fontSize);
    EXPECT_FLOAT_EQ(copy.letterSpacing, text.letterSpacing);
    EXPECT_FLOAT_EQ(copy.lineSpacing, text.lineSpacing);
}
#endif
