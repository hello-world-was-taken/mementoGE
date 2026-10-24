#include "core/ParticleSystem.h"
#include "core/SystemGraph.h"
#include "core/SystemScheduler.h"
#include "core/TextLayoutSystem.h"
#include "core/components/EntityInfo.h"
#include "core/components/ParticleEmitter.h"
#include "core/components/Text.h"
#include "core/components/TextAnchor.h"
#include "core/components/Transform.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace
{
class AuditedSystemsModes : public ::testing::TestWithParam<std::size_t>
{
};

std::shared_ptr<Font> makeCpuFont()
{
    auto font = std::make_shared<Font>(Font{});
    font->fontPath = "preloaded-cpu-test-font";
    font->pixelSize = 10.0f;
    font->metrics.ascent = 8.0f;
    font->metrics.descent = -2.0f;
    font->metrics.lineGap = 2.0f;
    font->metrics.valid = true;
    for (const char character : {'A', 'V'})
    {
        auto &glyph = font->packedChars[font->getGlyphIndex(character)];
        glyph.x0 = 1;
        glyph.y0 = 1;
        glyph.x1 = 7;
        glyph.y1 = 11;
        glyph.xoff = 0.0f;
        glyph.yoff = -8.0f;
        glyph.xoff2 = 6.0f;
        glyph.yoff2 = 2.0f;
        glyph.xadvance = 7.0f;
    }
    font->packedChars[font->getGlyphIndex(' ')].xadvance = 4.0f;
    font->metrics.kerning[font->getGlyphIndex('A') * Font::glyphCount + font->getGlyphIndex('V')] = -1.0f;
    return font;
}

void expectTransformUnchanged(const Transform &actual, const Transform &before)
{
    EXPECT_EQ(actual.position, before.position);
    EXPECT_EQ(actual.rotation, before.rotation);
    EXPECT_EQ(actual.scale, before.scale);
}

void expectFontUnchanged(const Font &actual, const Font &before)
{
    EXPECT_EQ(actual.fontPath, before.fontPath);
    EXPECT_EQ(actual.atlasTexture, nullptr);
    EXPECT_FLOAT_EQ(actual.pixelSize, before.pixelSize);
    EXPECT_EQ(actual.firstChar, before.firstChar);
    EXPECT_EQ(actual.charCount, before.charCount);
    EXPECT_EQ(actual.metrics.valid, before.metrics.valid);
    EXPECT_FLOAT_EQ(actual.metrics.ascent, before.metrics.ascent);
    EXPECT_FLOAT_EQ(actual.metrics.descent, before.metrics.descent);
    EXPECT_FLOAT_EQ(actual.metrics.lineGap, before.metrics.lineGap);
    EXPECT_EQ(actual.metrics.kerning, before.metrics.kerning);
    for (const char character : {'A', 'V', ' '})
    {
        SCOPED_TRACE(character);
        const auto &glyph = actual.getPackedChar(character);
        const auto &original = before.getPackedChar(character);
        EXPECT_EQ(glyph.x0, original.x0);
        EXPECT_EQ(glyph.y0, original.y0);
        EXPECT_EQ(glyph.x1, original.x1);
        EXPECT_EQ(glyph.y1, original.y1);
        EXPECT_FLOAT_EQ(glyph.xoff, original.xoff);
        EXPECT_FLOAT_EQ(glyph.yoff, original.yoff);
        EXPECT_FLOAT_EQ(glyph.xoff2, original.xoff2);
        EXPECT_FLOAT_EQ(glyph.yoff2, original.yoff2);
        EXPECT_FLOAT_EQ(glyph.xadvance, original.xadvance);
    }
}
} // namespace

TEST_P(AuditedSystemsModes, PreparedParticleAndTextUpdatesProduceDeterministicIndependentResults)
{
    entt::registry registry;
    Physics2D physics{glm::vec2{0.0f, -9.8f}};
    FrameContext frame;
    frame.deltaTime = 0.25f;
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 1, true}};
    ParticleSystem particles;
    TextLayoutSystem textLayout;
    const auto particleDescriptor = particles.descriptor();
    const auto textDescriptor = textLayout.descriptor();
    EXPECT_EQ(particleDescriptor.threadAffinity(), SystemThreadAffinity::AnyWorker);
    EXPECT_EQ(textDescriptor.threadAffinity(), SystemThreadAffinity::AnyWorker);
    EXPECT_FALSE(particleDescriptor.conflictsWith(textDescriptor));
    EXPECT_FALSE(textDescriptor.conflictsWith(particleDescriptor));
    const auto graph = SystemGraph::build({particleDescriptor, textDescriptor});
    EXPECT_TRUE(graph.successors().at("ParticleSystem").empty());
    EXPECT_TRUE(graph.successors().at("TextLayoutSystem").empty());

    const auto font = makeCpuFont();
    const Font originalFont = *font;
    const auto firstEntity = registry.create();
    auto &firstTransform = registry.emplace<Transform>(firstEntity);
    firstTransform.position = {50.0f, 80.0f, 2.0f};
    firstTransform.rotation = {0.0f, 0.0f, 17.0f};
    firstTransform.scale = {2.0f, 3.0f, 1.0f};
    const Transform originalFirstTransform = firstTransform;
    auto &firstInfo = registry.emplace<EntityInfo>(firstEntity);
    firstInfo.width = 100;
    firstInfo.height = 40;
    auto &firstAnchor = registry.emplace<TextAnchor>(firstEntity);
    firstAnchor.anchor = TextAnchorPoint::TopRight;
    firstAnchor.offset = {3.0f, -2.0f};
    auto &firstText = registry.emplace<Text>(firstEntity);
    firstText.content = "AV";
    firstText.font = font;
    firstText.fontSize = 10.0f;
    firstText.letterSpacing = 2.0f;
    firstText.localOffset = {1000.0f, 1000.0f};

    // Both systems read this entity's Transform while mutating separate pools.
    auto &emitter = registry.emplace<ParticleEmitter>(firstEntity);
    emitter.rate = 0.0f;
    emitter.accumulator = 0.25f;
    emitter.particles = {
        Particle{{2.0f, 4.0f, 6.0f}, {4.0f, -2.0f, 1.0f}, 1.0f, 2.0f, 3.0f, {0.1f, 0.2f, 0.3f, 0.9f}},
        Particle{{9.0f, 9.0f, 9.0f}, {1.0f, 1.0f, 1.0f}, 0.1f, 1.0f, 2.0f, {1.0f, 1.0f, 1.0f, 1.0f}},
        Particle{{-1.0f, 2.0f, 0.0f}, {0.0f, 8.0f, -4.0f}, 2.0f, 2.0f, 4.0f, {0.4f, 0.5f, 0.6f, 0.2f}}};

    const auto secondEntity = registry.create();
    auto &secondTransform = registry.emplace<Transform>(secondEntity);
    secondTransform.position = {-20.0f, 10.0f, 5.0f};
    secondTransform.rotation = {0.0f, 0.0f, -23.0f};
    secondTransform.scale = {0.5f, 0.75f, 1.0f};
    const Transform originalSecondTransform = secondTransform;
    auto &secondInfo = registry.emplace<EntityInfo>(secondEntity);
    secondInfo.width = 60;
    secondInfo.height = 50;
    auto &secondAnchor = registry.emplace<TextAnchor>(secondEntity);
    secondAnchor.anchor = TextAnchorPoint::BottomLeft;
    secondAnchor.offset = {-1.0f, 4.0f};
    auto &secondText = registry.emplace<Text>(secondEntity);
    secondText.content = "A\nA";
    secondText.font = font;
    secondText.fontSize = 10.0f;
    secondText.lineSpacing = 3.0f;
    secondText.localOffset = {1000.0f, 1000.0f};

    particleDescriptor.prepareRegistry(registry);
    textDescriptor.prepareRegistry(registry);
    const std::map<std::string, std::reference_wrapper<ISystem>> enabled{
        {"ParticleSystem", particles}, {"TextLayoutSystem", textLayout}};

    const auto timings = scheduler.run(graph, enabled, registry, physics, frame);

    EXPECT_EQ(timings.size(), 2u);
    EXPECT_EQ(timings.count("ParticleSystem"), 1u);
    EXPECT_EQ(timings.count("TextLayoutSystem"), 1u);
    const auto &updatedFirstText = registry.get<Text>(firstEntity);
    const auto &updatedSecondText = registry.get<Text>(secondEntity);
    EXPECT_EQ(updatedFirstText.localOffset, (glm::vec2{38.0f, 8.0f}));
    EXPECT_EQ(updatedSecondText.localOffset, (glm::vec2{-31.0f, -21.0f}));
    EXPECT_EQ(updatedFirstText.font, font);
    EXPECT_EQ(updatedSecondText.font, font);
    EXPECT_TRUE(updatedFirstText.vertices.empty());
    EXPECT_TRUE(updatedSecondText.vertices.empty());
    expectTransformUnchanged(registry.get<Transform>(firstEntity), originalFirstTransform);
    expectTransformUnchanged(registry.get<Transform>(secondEntity), originalSecondTransform);
    expectFontUnchanged(*font, originalFont);

    const auto &updatedEmitter = registry.get<ParticleEmitter>(firstEntity);
    EXPECT_FLOAT_EQ(updatedEmitter.accumulator, 0.25f);
    ASSERT_EQ(updatedEmitter.particles.size(), 2u);
    const auto firstParticle = std::find_if(updatedEmitter.particles.begin(), updatedEmitter.particles.end(),
        [](const Particle &particle) { return particle.size == 3.0f; });
    const auto secondParticle = std::find_if(updatedEmitter.particles.begin(), updatedEmitter.particles.end(),
        [](const Particle &particle) { return particle.size == 4.0f; });
    ASSERT_NE(firstParticle, updatedEmitter.particles.end());
    ASSERT_NE(secondParticle, updatedEmitter.particles.end());
    EXPECT_EQ(firstParticle->pos, (glm::vec3{3.0f, 3.5f, 6.25f}));
    EXPECT_FLOAT_EQ(firstParticle->life, 0.75f);
    EXPECT_EQ(firstParticle->color, (glm::vec4{0.1f, 0.2f, 0.3f, 0.375f}));
    EXPECT_EQ(secondParticle->pos, (glm::vec3{-1.0f, 4.0f, -1.0f}));
    EXPECT_FLOAT_EQ(secondParticle->life, 1.75f);
    EXPECT_EQ(secondParticle->color, (glm::vec4{0.4f, 0.5f, 0.6f, 0.875f}));
}

INSTANTIATE_TEST_SUITE_P(WorkerCounts, AuditedSystemsModes, ::testing::Values(0u, 1u, 2u));
