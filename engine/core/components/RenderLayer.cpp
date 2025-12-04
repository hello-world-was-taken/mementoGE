#include "core/components/RenderLayer.h"

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"
#include <imgui.h>
#include <yaml-cpp/yaml.h>
#endif

#ifdef EDITOR_BUILD

std::string RenderLayer::renderLayerTypeToString(RenderLayerType type)
{
    switch (type)
    {
    case RenderLayerType::Background:
        return "Background";
    case RenderLayerType::World:
        return "World";
    case RenderLayerType::Foreground:
        return "Foreground";
    case RenderLayerType::UI:
        return "UI";
    case RenderLayerType::Debug:
        return "Debug";
    }
    return "World";
}

RenderLayerType RenderLayer::renderLayerTypeFromString(const std::string &s)
{
    if (s == "Background")
    {
        return RenderLayerType::Background;
    }
    if (s == "World")
    {
        return RenderLayerType::World;
    }
    if (s == "Foreground")
    {
        return RenderLayerType::Foreground;
    }
    if (s == "UI")
    {
        return RenderLayerType::UI;
    }
    if (s == "Debug")
    {
        return RenderLayerType::Debug;
    }

    return RenderLayerType::World; // default fallback
}

void RenderLayer::serialize(YAML::Emitter &out) const
{
    out << YAML::Key << "RenderLayer";
    out << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "Layer" << YAML::Value << renderLayerTypeToString(layer);
    out << YAML::Key << "Order" << YAML::Value << order;
    out << YAML::Key << "Mask" << YAML::Value << mask;

    out << YAML::EndMap;
}

void RenderLayer::deserialize(const YAML::Node &in)
{
    const YAML::Node &n = in["RenderLayer"];
    if (!n)
    {
        return;
    }

    layer = renderLayerTypeFromString(n["Layer"].as<std::string>());
    order = n["Order"].as<int>();
    mask = n["Mask"].as<uint32_t>();
}

void RenderLayer::drawInspector()
{
    ImGuiWrapper::Collapsable("Render Layer",
        [&]
        {
            const char *layerNames[] = {"Background", "World", "Foreground", "UI", "Debug"};
            int current = static_cast<int>(layer);

            if (ImGui::Combo("Layer", &current, layerNames, IM_ARRAYSIZE(layerNames)))
            {
                layer = static_cast<RenderLayerType>(current);
            }

            ImGui::DragInt("Order", &order, 1.0f, -1000, 1000);
            ImGui::InputScalar(
                "Mask", ImGuiDataType_U32, &mask, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
        });
}

#endif
