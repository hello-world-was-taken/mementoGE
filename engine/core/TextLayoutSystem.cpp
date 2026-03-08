#include "core/TextLayoutSystem.h"

#include "core/components/EntityInfo.h"
#include "core/components/Text.h"
#include "core/components/TextAnchor.h"
#include "core/components/Transform.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace
{
// Approximate the rendered text size in world units using the
// same per-glyph sizing logic as Text::rebuild. This lets the
// layout system position text so that the whole block stays
// within the owning game object's bounds for different anchor points.
glm::vec2 computeTextSize(const Text &text)
{
    if (!text.font || text.content.empty())
    {
        return {0.0f, 0.0f};
    }

    float totalWidth = 0.0f;
    float maxHeight = 0.0f;

    for (char ch : text.content)
    {
        const stbtt_packedchar &packedChar = text.font->getPackedChar(ch);

        float w = static_cast<float>(packedChar.x1 - packedChar.x0);
        float h = static_cast<float>(packedChar.y1 - packedChar.y0);

        float aspect = 1.0f;
        if (h > 0.0f)
        {
            aspect = w / h;
        }

        float glyphSize = text.fontSize * aspect;
        totalWidth += glyphSize;
        maxHeight = std::max(maxHeight, text.fontSize);
    }

    return {totalWidth, maxHeight};
}

// Compute the bottom-left position of the text block inside the
// game object's rectangle, using the object's size (EntityInfo.width/height)
// as the reference. The rectangle is centered around the Transform position.
glm::vec2 computeAnchorPosition(
    TextAnchorPoint anchor, const glm::vec2 &textSize, const glm::vec2 &objectCenter, const glm::vec2 &objectSize)
{
    const float objW = objectSize.x;
    const float objH = objectSize.y;

    const float textW = textSize.x;
    const float textH = textSize.y;

    const float halfW = objW * 0.5f;
    const float halfH = objH * 0.5f;

    const float left = objectCenter.x - halfW;
    const float right = objectCenter.x + halfW;
    const float bottom = objectCenter.y - halfH;
    const float top = objectCenter.y + halfH;

    switch (anchor)
    {
    case TextAnchorPoint::TopLeft:
        return {left, top - textH};
    case TextAnchorPoint::TopCenter:
        return {objectCenter.x - textW * 0.5f, top - textH};
    case TextAnchorPoint::TopRight:
        return {right - textW, top - textH};
    case TextAnchorPoint::CenterLeft:
        return {left, objectCenter.y - textH * 0.5f};
    case TextAnchorPoint::Center:
        return {objectCenter.x - textW * 0.5f, objectCenter.y - textH * 0.5f};
    case TextAnchorPoint::CenterRight:
        return {right - textW, objectCenter.y - textH * 0.5f};
    case TextAnchorPoint::BottomLeft:
        return {left, bottom};
    case TextAnchorPoint::BottomCenter:
        return {objectCenter.x - textW * 0.5f, bottom};
    case TextAnchorPoint::BottomRight:
        return {right - textW, bottom};
    default:
        return {left, top - textH};
    }
}
} // namespace

void TextLayoutSystem::update(entt::registry &registry)
{
    // For every entity that has Text + TextAnchor + Transform, compute
    // where the text should appear within the game object's own bounds
    // and store that as a local offset on the Text component. This way
    // we do not move the game object itself, only the text relative to it.
    auto view = registry.view<Transform, Text, TextAnchor, EntityInfo>();

    for (entt::entity entity : view)
    {
        auto &transform = view.get<Transform>(entity);
        auto &text = view.get<Text>(entity);
        auto &anchorComp = view.get<TextAnchor>(entity);
        auto &info = view.get<EntityInfo>(entity);

        glm::vec2 textSize = computeTextSize(text);
        glm::vec2 objectCenter{transform.position.x, transform.position.y};
        glm::vec2 objectSize{static_cast<float>(info.width), static_cast<float>(info.height)};

        // World-space bottom-left position for the text block inside
        // the object's rectangle, including the anchor's extra offset.
        glm::vec2 basePos = computeAnchorPosition(anchorComp.anchor, textSize, objectCenter, objectSize);
        glm::vec2 finalPos = basePos + anchorComp.offset;

        // Store as a local offset from the object's center so we don't
        // change the Transform, which would move the whole game object.
        text.localOffset = finalPos - objectCenter;
    }
}
