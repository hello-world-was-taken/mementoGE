#include "core/components/Text.h"
#include "core/components/Transform.h"

#include "core/AssetManager.h"

#include "opengl/Vertex.h"

#include "editor/DragNDropPayloads.h" // FIXME: core depending on editor

#include <algorithm>
#include <iostream>

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"

#include <imgui.h>
#include <yaml-cpp/yaml.h>
#endif

namespace
{
struct GlyphLayout
{
    char character;
    float left;
    float right;
    float bottom;
    float top;
};

struct TextGeometryLayout
{
    std::vector<GlyphLayout> glyphs;
    glm::vec2 boundsMin{0.0f, 0.0f};
    glm::vec2 size{0.0f, 0.0f};
};

std::size_t nextCharacterIndex(const std::string &content, std::size_t index)
{
    ++index;
    while (index < content.size() && content[index] == '\r')
    {
        ++index;
    }
    return index;
}

TextGeometryLayout layoutText(const Text &text)
{
    TextGeometryLayout layout;
    if (!text.font || text.content.empty() || text.fontSize <= 0.0f || text.font->pixelSize <= 0.0f)
    {
        return layout;
    }

    const Font &font = *text.font;
    const float scale = text.fontSize / font.pixelSize;
    const float ascent = (font.metrics.valid ? font.metrics.ascent : font.pixelSize) * scale;
    const float descent = (font.metrics.valid ? font.metrics.descent : 0.0f) * scale;
    const float lineGap = (font.metrics.valid ? font.metrics.lineGap : 0.0f) * scale;
    const float glyphLineHeight = std::max(text.fontSize, ascent - descent);
    const float lineAdvance = glyphLineHeight + lineGap + text.lineSpacing;
    const std::size_t lineCount = 1 + static_cast<std::size_t>(std::count(text.content.begin(), text.content.end(), '\n'));
    const float blockHeight = glyphLineHeight + static_cast<float>(lineCount - 1) * lineAdvance;

    float minX = 0.0f;
    float maxX = 0.0f;
    float minY = 0.0f;
    float maxY = blockHeight;
    float penX = 0.0f;
    float baselineY = blockHeight - ascent;

    const auto finishLine = [&]
    {
        minX = std::min(minX, penX);
        maxX = std::max(maxX, penX);
    };

    for (std::size_t index = 0; index < text.content.size(); ++index)
    {
        const char ch = text.content[index];
        if (ch == '\r')
        {
            continue;
        }
        if (ch == '\n')
        {
            finishLine();
            penX = 0.0f;
            baselineY -= lineAdvance;
            continue;
        }

        const std::size_t nextIndex = nextCharacterIndex(text.content, index);
        const bool hasNextOnLine = nextIndex < text.content.size() && text.content[nextIndex] != '\n';

        if (ch == '\t')
        {
            penX += font.getPackedChar(' ').xadvance * scale * 4.0f;
            if (hasNextOnLine)
            {
                penX += text.letterSpacing;
            }
            continue;
        }

        const stbtt_packedchar &packedChar = font.getPackedChar(ch);
        const float left = penX + packedChar.xoff * scale;
        const float right = penX + packedChar.xoff2 * scale;
        const float top = baselineY - packedChar.yoff * scale;
        const float bottom = baselineY - packedChar.yoff2 * scale;

        // Spaces and a few control/fallback glyphs can have no bitmap. Their
        // advance still contributes to measurement, but no degenerate quad is emitted.
        if (packedChar.x1 > packedChar.x0 && packedChar.y1 > packedChar.y0)
        {
            layout.glyphs.push_back({ch, left, right, bottom, top});
            minX = std::min(minX, left);
            maxX = std::max(maxX, right);
            minY = std::min(minY, bottom);
            maxY = std::max(maxY, top);
        }

        penX += packedChar.xadvance * scale;
        if (hasNextOnLine)
        {
            penX += font.getKerning(ch, text.content[nextIndex]) * scale + text.letterSpacing;
        }
    }

    finishLine();
    layout.boundsMin = {minX, minY};
    layout.size = {maxX - minX, maxY - minY};
    return layout;
}
} // namespace

glm::vec2 Text::measure() const
{
    return layoutText(*this).size;
}

// FIXME: for the moment rebuild gets called every frame in sprite renderer.
void Text::rebuild(Transform &transform)
{
    vertices.clear();

    if (!font)
    {
        std::cerr << "Please select a font!" << std::endl;
        return;
    }

    const TextGeometryLayout layout = layoutText(*this);
    const glm::vec3 origin = transform.position + glm::vec3(localOffset - layout.boundsMin, 0.0f);

    for (const GlyphLayout &glyph : layout.glyphs)
    {
        const stbtt_aligned_quad &alignedQuad = font->getAlignedQuad(glyph.character);

        // The order of vertices of a quad goes top-right, top-left, bottom-left, bottom-right
        const glm::vec3 glyphVertices[4] = {
            {origin.x + glyph.right, origin.y + glyph.top, origin.z},
            {origin.x + glyph.left, origin.y + glyph.top, origin.z},
            {origin.x + glyph.left, origin.y + glyph.bottom, origin.z},
            {origin.x + glyph.right, origin.y + glyph.bottom, origin.z}};

        glm::vec2 glyphTextureCoords[4] = {
            {alignedQuad.s1, alignedQuad.t0},
            {alignedQuad.s0, alignedQuad.t0},
            {alignedQuad.s0, alignedQuad.t1},
            {alignedQuad.s1, alignedQuad.t1},
        };

        auto makeVertex = [&](const glm::vec3 &pos, const glm::vec4 &color, const glm::vec2 &uv, int t)
        {
            return Vertex{pos, color, uv, t};
        };

        // The renderer replaces this placeholder with the atlas's slot in the
        // current draw batch.
        constexpr int texSlot = 0;
        vertices.push_back(makeVertex(glyphVertices[0], color, glyphTextureCoords[0], texSlot));
        vertices.push_back(makeVertex(glyphVertices[1], color, glyphTextureCoords[1], texSlot));
        vertices.push_back(makeVertex(glyphVertices[2], color, glyphTextureCoords[2], texSlot));
        vertices.push_back(makeVertex(glyphVertices[3], color, glyphTextureCoords[3], texSlot));
    }
}

#ifdef EDITOR_BUILD
void Text::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "Text" << YAML::BeginMap;

    out << YAML::Key << "Content" << YAML::Value << content;
    out << YAML::Key << "FontSize" << YAML::Value << fontSize;
    out << YAML::Key << "LetterSpacing" << YAML::Value << letterSpacing;
    out << YAML::Key << "LineSpacing" << YAML::Value << lineSpacing;

    out << YAML::Key << "Color" << YAML::Value << YAML::Flow << YAML::BeginSeq << color.r << color.g << color.b
        << color.a << YAML::EndSeq;

    if (font)
    {
        out << YAML::Key << "Font" << YAML::Value << font->fontPath;
    }

    out << YAML::EndMap;
}

void Text::deserialize(const YAML::Node &in)
{
    if (!in["Text"])
    {
        return;
    }

    const YAML::Node textNode = in["Text"];

    if (textNode["Content"])
    {
        content = textNode["Content"].as<std::string>();
    }

    if (textNode["FontSize"])
    {
        fontSize = textNode["FontSize"].as<float>();
    }

    if (textNode["LetterSpacing"])
    {
        letterSpacing = textNode["LetterSpacing"].as<float>();
    }

    if (textNode["LineSpacing"])
    {
        lineSpacing = textNode["LineSpacing"].as<float>();
    }

    if (textNode["Color"])
    {
        auto c = textNode["Color"];
        color = {c[0].as<float>(), c[1].as<float>(), c[2].as<float>(), c[3].as<float>()};
    }

    if (textNode["Font"])
    {
        std::string fontPath = textNode["Font"].as<std::string>();
        font = AssetManager::instance().getFont(fontPath);
    }
}

void Text::drawInspector()
{
    ImGuiWrapper::Collapsable("Text",
        [&]
        {
            if (ImGuiWrapper::InputTextSimple("Content", content))
            {
                needsRebuild = true;
            }

            if (ImGui::DragFloat("Font Size", &fontSize, 0.5f, 1.0f, 512.0f))
            {
                needsRebuild = true;
            }

            if (ImGui::DragFloat("Letter Spacing", &letterSpacing, 0.1f, -64.0f, 64.0f))
            {
                needsRebuild = true;
            }

            if (ImGui::DragFloat("Line Spacing", &lineSpacing, 0.1f, -64.0f, 128.0f))
            {
                needsRebuild = true;
            }

            if (ImGui::ColorEdit4("Color##Text", &color.x))
            {
                needsRebuild = true;
            }

            ImGui::Separator();
            ImGui::Text("Font: %s", font ? font->fontPath.c_str() : "None");

            // Font drag & drop
            // TODO: the drag area is too small. Only the above font text. Might be better
            // to use begin child to make the whole area droppable.
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("FONT"))
                {
                    IM_ASSERT(payload->DataSize == sizeof(FontPayload));
                    FontPayload fontPayload = *(FontPayload *)payload->Data;

                    font = AssetManager::instance().getFont(fontPayload.fontPath);

                    needsRebuild = true;
                }
                ImGui::EndDragDropTarget();
            }
        });
}
#endif
