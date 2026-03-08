#include "core/components/TextAnchor.h"

#ifdef EDITOR_BUILD

#include "core/ImGuiWrapper.h"

#include <imgui.h>
#include <yaml-cpp/yaml.h>

void TextAnchor::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "TextAnchor" << YAML::BeginMap;

    out << YAML::Key << "Anchor" << YAML::Value << static_cast<int>(anchor);
    out << YAML::Key << "Offset" << YAML::Value << YAML::Flow << YAML::BeginSeq << offset.x << offset.y << YAML::EndSeq;

    out << YAML::EndMap;
}

void TextAnchor::deserialize(const YAML::Node &in)
{
    if (!in["TextAnchor"])
    {
        return;
    }

    const YAML::Node node = in["TextAnchor"];

    if (node["Anchor"])
    {
        int value = node["Anchor"].as<int>();
        if (value < static_cast<int>(TextAnchorPoint::TopLeft) ||
            value > static_cast<int>(TextAnchorPoint::BottomRight))
        {
            value = static_cast<int>(TextAnchorPoint::TopLeft);
        }
        anchor = static_cast<TextAnchorPoint>(value);
    }

    if (node["Offset"] && node["Offset"].IsSequence() && node["Offset"].size() == 2)
    {
        offset.x = node["Offset"][0].as<float>();
        offset.y = node["Offset"][1].as<float>();
    }
}

void TextAnchor::drawInspector()
{
    ImGuiWrapper::Collapsable("Text Anchor",
        [&]
        {
            const char *items[] = {
                "Top Left",
                "Top Center",
                "Top Right",
                "Center Left",
                "Center",
                "Center Right",
                "Bottom Left",
                "Bottom Center",
                "Bottom Right",
            };

            int current = static_cast<int>(anchor);
            if (ImGui::Combo("Anchor", &current, items, IM_ARRAYSIZE(items)))
            {
                if (current < static_cast<int>(TextAnchorPoint::TopLeft) ||
                    current > static_cast<int>(TextAnchorPoint::BottomRight))
                {
                    current = static_cast<int>(TextAnchorPoint::TopLeft);
                }
                anchor = static_cast<TextAnchorPoint>(current);
            }

            if (ImGui::DragFloat2("Offset", &offset.x, 1.0f))
            {
                // no-op: offset is applied by the layout system each frame
            }
        });
}

#endif
