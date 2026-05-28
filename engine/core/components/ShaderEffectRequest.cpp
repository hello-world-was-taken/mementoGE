#include "core/components/ShaderEffectRequest.h"

#include <yaml-cpp/yaml.h>

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"

#include <imgui.h>
#endif

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

#ifdef EDITOR_BUILD
void ShaderEffectRequest::drawInspector()
{
    ImGuiWrapper::Collapsable("Shader Effect Request",
        [&]
        {
            ImGuiWrapper::InputTextSimple("Vertex Shader Path", vertexShaderPath);
            ImGuiWrapper::InputTextSimple("Fragment Shader Path", fragmentShaderPath);
        });
}
#endif
