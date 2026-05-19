#include "core/components/ShaderEffectRequest.h"

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"

#include <imgui.h>
#include <yaml-cpp/yaml.h>

void ShaderEffectRequest::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "ShaderEffectRequest" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "vertexShaderPath" << YAML::Value << vertexShaderPath;
    out << YAML::Key << "fragmentShaderPath" << YAML::Value << fragmentShaderPath;
    out << YAML::EndMap;
}

void ShaderEffectRequest::deserialize(const YAML::Node &node)
{
    if (!node["ShaderEffectRequest"])
    {
        return;
    }

    const auto &data = node["ShaderEffectRequest"];
    if (data["vertexShaderPath"])
    {
        vertexShaderPath = data["vertexShaderPath"].as<std::string>();
    }
    if (data["fragmentShaderPath"])
    {
        fragmentShaderPath = data["fragmentShaderPath"].as<std::string>();
    }
}

void ShaderEffectRequest::drawInspector()
{
    ImGuiWrapper::Collapsable("Shader Effect Request",
        [&]
        {
            ImGui::InputText("Vertex Shader Path", &vertexShaderPath);
            ImGui::InputText("Fragment Shader Path", &fragmentShaderPath);
        });
}

#endif