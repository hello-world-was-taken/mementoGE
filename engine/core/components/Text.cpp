#include "core/components/Text.h"
#include "core/components/Transform.h"

#include "core/AssetManager.h"

#include "opengl/Vertex.h"

#include "editor/DragNDropPayloads.h" // FIXME: core depending on editor

#include <iostream>

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"

#include <imgui.h>
#include <yaml-cpp/yaml.h>
#endif

// FIXME: for the moment rebuild gets called everyframe in sprite renderer.
// Also, small letters don't align with the baseline. But that is okay for
// the current usecase.
void Text::rebuild(Transform &transform)
{
    vertices.clear();

    if (!font)
    {
        std::cerr << "Please select a font!" << std::endl;
        return;
    }

    glm::vec3 position = transform.position;
    for (const char &ch : content)
    {
        const stbtt_packedchar &packedChar = font->getPackedChar(ch);
        const stbtt_aligned_quad &alignedQuad = font->getAlignedQuad(ch);

        float aspectRatio = 1;
        float w = static_cast<float>(packedChar.x1 - packedChar.x0);
        float h = static_cast<float>(packedChar.y1 - packedChar.y0);

        if (h > 0.0f) // division by zero check
        {
            float aspectRatio = w / h;
        }

        auto size = fontSize * aspectRatio;

        // The order of vertices of a quad goes top-right, top-left, bottom-left, bottom-right
        glm::vec3 glyphVertices[4] = {{position.x + size, position.y + size, 0},
            {position.x, position.y + size, 0},
            {position.x, position.y, 0},
            {position.x + size, position.y, 0}};

        glm::vec2 glyphTextureCoords[4] = {
            {alignedQuad.s1, alignedQuad.t0},
            {alignedQuad.s0, alignedQuad.t0},
            {alignedQuad.s0, alignedQuad.t1},
            {alignedQuad.s1, alignedQuad.t1},
        };

        auto makeVertex = [&](const glm::vec3 &pos, const glm::vec4 &color, const glm::vec2 &uv, float t)
        {
            return Vertex{pos, color, uv, t};
        };

        auto texUnit = font->atlasTexture->getTextureUnit();
        vertices.push_back(makeVertex(glyphVertices[0], color, glyphTextureCoords[0], texUnit));
        vertices.push_back(makeVertex(glyphVertices[1], color, glyphTextureCoords[1], texUnit));
        vertices.push_back(makeVertex(glyphVertices[2], color, glyphTextureCoords[2], texUnit));
        vertices.push_back(makeVertex(glyphVertices[3], color, glyphTextureCoords[3], texUnit));

        position += glm::vec3(size, 0.0f, 0.0f);
    }
}

#ifdef EDITOR_BUILD
void Text::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "Text" << YAML::BeginMap;

    out << YAML::Key << "Content" << YAML::Value << content;
    out << YAML::Key << "FontSize" << YAML::Value << fontSize;

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