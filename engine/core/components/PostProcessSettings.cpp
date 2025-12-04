#include "core/components/PostProcessSettings.h"
#include "core/components/RenderLayer.h"

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"
#include <imgui.h>
#endif

#ifdef EDITOR_BUILD
void PostProcessSettings::serialize(YAML::Emitter &out) const
{
    out << YAML::Key << "PostProcessSettings";
    out << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "Enabled" << YAML::Value << enabled;

    // Blur
    out << YAML::Key << "Blur" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Enabled" << YAML::Value << blur.enabled;
    out << YAML::Key << "Radius" << YAML::Value << blur.radius;

    out << YAML::Key << "TargetLayers" << YAML::Value << YAML::BeginSeq;
    for (auto layer : blur.targetLayers)
    {
        out << RenderLayer::renderLayerTypeToString(layer);
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;

    // Bloom
    out << YAML::Key << "Bloom" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Enabled" << YAML::Value << bloom.enabled;
    out << YAML::Key << "Threshold" << YAML::Value << bloom.threshold;

    out << YAML::Key << "TargetLayers" << YAML::Value << YAML::BeginSeq;
    for (auto layer : bloom.targetLayers)
    {
        out << RenderLayer::renderLayerTypeToString(layer);
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;

    // Color grading
    out << YAML::Key << "ColorGrading" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Enabled" << YAML::Value << grade.enabled;
    out << YAML::Key << "Saturation" << YAML::Value << grade.saturation;

    out << YAML::Key << "TargetLayers" << YAML::Value << YAML::BeginSeq;
    for (auto layer : grade.targetLayers)
    {
        out << RenderLayer::renderLayerTypeToString(layer);
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;

    out << YAML::EndMap;
}

void PostProcessSettings::deserialize(const YAML::Node &in)
{
    const YAML::Node n = in["PostProcessSettings"];
    if (!n)
    {
        return;
    }

    enabled = n["Enabled"].as<bool>();

    // Blur
    auto bn = n["Blur"];
    blur.enabled = bn["Enabled"].as<bool>();
    blur.radius = bn["Radius"].as<float>();
    blur.targetLayers.clear();
    for (auto layerNode : bn["TargetLayers"])
    {
        blur.targetLayers.push_back(RenderLayer::renderLayerTypeFromString(layerNode.as<std::string>()));
    }

    // Bloom
    auto bln = n["Bloom"];
    bloom.enabled = bln["Enabled"].as<bool>();
    bloom.threshold = bln["Threshold"].as<float>();
    bloom.targetLayers.clear();
    for (auto layerNode : bln["TargetLayers"])
    {
        bloom.targetLayers.push_back(RenderLayer::renderLayerTypeFromString(layerNode.as<std::string>()));
    }

    // Color grading
    auto gn = n["ColorGrading"];
    grade.enabled = gn["Enabled"].as<bool>();
    grade.saturation = gn["Saturation"].as<float>();
    grade.targetLayers.clear();
    for (auto layerNode : gn["TargetLayers"])
    {
        grade.targetLayers.push_back(RenderLayer::renderLayerTypeFromString(layerNode.as<std::string>()));
    }
}

// GUI Helper: Multi-layer selection widget
static void drawLayerSelector(const char *postProcessingType, std::vector<RenderLayerType> &layers)
{
    // TODO: we should have these as a constant somewhere and reuse them in RenderLayer.h as well
    const char *layerNames[] = {"Background", "World", "Foreground", "UI", "Debug"};
    RenderLayerType layerValues[] = {RenderLayerType::Background, RenderLayerType::World, RenderLayerType::Foreground,
        RenderLayerType::UI, RenderLayerType::Debug};

    std::string comboName = std::string("Target Layers") + "##" + postProcessingType;
    if (ImGui::BeginCombo(comboName.c_str(), "Selected Layers"))
    {
        for (int i = 0; i < 5; i++)
        {
            std::string selectableName = std::string(layerNames[i]) + "##" + postProcessingType;
            std::cout << selectableName << std::endl;
            bool selected = (std::find(layers.begin(), layers.end(), layerValues[i]) != layers.end());
            if (ImGui::Selectable(selectableName.c_str(), selected))
            {
                if (selected)
                {
                    layers.erase(std::remove(layers.begin(), layers.end(), layerValues[i]), layers.end());
                }
                else
                {
                    layers.push_back(layerValues[i]);
                }
            }
        }
        ImGui::EndCombo();
    }
}

void PostProcessSettings::drawInspector()
{
    ImGuiWrapper::Collapsable("Post Processing",
        [&]
        {
            ImGui::Checkbox("Enabled", &enabled);

            ImGuiWrapper::Collapsable("Blur",
                [&]
                {
                    ImGui::Checkbox("Blur Enabled", &blur.enabled);
                    ImGui::DragFloat("Radius", &blur.radius, 0.05f, 0.0f, 100.0f);

                    drawLayerSelector("Blur", blur.targetLayers);
                });

            ImGuiWrapper::Collapsable("Bloom",
                [&]
                {
                    ImGui::Checkbox("Bloom Enabled", &bloom.enabled);
                    ImGui::DragFloat("Threshold", &bloom.threshold, 0.05f, 0.0f, 10.0f);

                    drawLayerSelector("Bloom", bloom.targetLayers);
                });

            ImGuiWrapper::Collapsable("Color Grading",
                [&]
                {
                    ImGui::Checkbox("Grading Enabled", &grade.enabled);
                    ImGui::DragFloat("Saturation", &grade.saturation, 0.02f, 0.0f, 5.0f);

                    drawLayerSelector("Color Grading", grade.targetLayers);
                });
        });
}

#endif
