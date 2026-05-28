#include "core/components/ShaderEffectRequest.h"

#include <algorithm>
#include <type_traits>
#include <vector>
#include <yaml-cpp/yaml.h>

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"

#include <imgui.h>
#endif

namespace
{
template<typename Vector>
void serializeVector(YAML::Emitter &out, const Vector &vector)
{
    out << YAML::Flow << YAML::BeginSeq;
    for (glm::length_t index = 0; index < vector.length(); ++index)
    {
        out << vector[index];
    }
    out << YAML::EndSeq;
}

template<typename Vector>
bool deserializeVector(const YAML::Node &node, Vector &vector)
{
    if (!node.IsSequence() || node.size() != vector.length())
    {
        return false;
    }

    for (glm::length_t index = 0; index < vector.length(); ++index)
    {
        vector[index] = node[index].as<float>();
    }
    return true;
}

bool deserializeUniform(const YAML::Node &node, ShaderUniformValue &uniform)
{
    if (!node.IsMap() || !node["type"] || !node["value"])
    {
        return false;
    }

    const std::string type = node["type"].as<std::string>();
    const YAML::Node value = node["value"];
    if (type == "int")
    {
        uniform = value.as<int>();
        return true;
    }
    if (type == "float")
    {
        uniform = value.as<float>();
        return true;
    }
    if (type == "vec2")
    {
        glm::vec2 vector;
        if (deserializeVector(value, vector))
        {
            uniform = vector;
            return true;
        }
        return false;
    }
    if (type == "vec3")
    {
        glm::vec3 vector;
        if (deserializeVector(value, vector))
        {
            uniform = vector;
            return true;
        }
        return false;
    }
    if (type == "vec4")
    {
        glm::vec4 vector;
        if (deserializeVector(value, vector))
        {
            uniform = vector;
            return true;
        }
        return false;
    }
    if (type == "mat4")
    {
        if (!value.IsSequence() || value.size() != 4)
        {
            return false;
        }

        glm::mat4 matrix{1.0F};
        for (glm::length_t column = 0; column < 4; ++column)
        {
            if (!deserializeVector(value[column], matrix[column]))
            {
                return false;
            }
        }
        uniform = matrix;
        return true;
    }
    return false;
}
}

void ShaderEffectRequest::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "ShaderEffectRequest" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "vertexShaderPath" << YAML::Value << vertexShaderPath;
    out << YAML::Key << "fragmentShaderPath" << YAML::Value << fragmentShaderPath;
    out << YAML::Key << "uniforms" << YAML::Value << YAML::BeginMap;

    std::vector<std::string> uniformNames;
    uniformNames.reserve(uniforms.size());
    for (const auto &entry : uniforms)
    {
        uniformNames.push_back(entry.first);
    }
    std::sort(uniformNames.begin(), uniformNames.end());

    for (const std::string &name : uniformNames)
    {
        out << YAML::Key << name << YAML::Value << YAML::BeginMap;
        std::visit(
            [&out](const auto &value)
            {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Value, int>)
                {
                    out << YAML::Key << "type" << YAML::Value << "int";
                    out << YAML::Key << "value" << YAML::Value << value;
                }
                else if constexpr (std::is_same_v<Value, float>)
                {
                    out << YAML::Key << "type" << YAML::Value << "float";
                    out << YAML::Key << "value" << YAML::Value << value;
                }
                else if constexpr (std::is_same_v<Value, glm::vec2>)
                {
                    out << YAML::Key << "type" << YAML::Value << "vec2";
                    out << YAML::Key << "value" << YAML::Value;
                    serializeVector(out, value);
                }
                else if constexpr (std::is_same_v<Value, glm::vec3>)
                {
                    out << YAML::Key << "type" << YAML::Value << "vec3";
                    out << YAML::Key << "value" << YAML::Value;
                    serializeVector(out, value);
                }
                else if constexpr (std::is_same_v<Value, glm::vec4>)
                {
                    out << YAML::Key << "type" << YAML::Value << "vec4";
                    out << YAML::Key << "value" << YAML::Value;
                    serializeVector(out, value);
                }
                else if constexpr (std::is_same_v<Value, glm::mat4>)
                {
                    out << YAML::Key << "type" << YAML::Value << "mat4";
                    out << YAML::Key << "value" << YAML::Value << YAML::BeginSeq;
                    for (glm::length_t column = 0; column < 4; ++column)
                    {
                        serializeVector(out, value[column]);
                    }
                    out << YAML::EndSeq;
                }
            },
            uniforms.at(name));
        out << YAML::EndMap;
    }

    out << YAML::EndMap;
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

    uniforms.clear();
    const YAML::Node serializedUniforms = data["uniforms"];
    if (!serializedUniforms || !serializedUniforms.IsMap())
    {
        return;
    }

    for (const auto &entry : serializedUniforms)
    {
        try
        {
            const std::string name = entry.first.as<std::string>();
            ShaderUniformValue uniform;
            if (deserializeUniform(entry.second, uniform))
            {
                uniforms.insert_or_assign(name, std::move(uniform));
            }
        }
        catch (const YAML::Exception &)
        {
            // Ignore malformed or unsupported uniforms without losing the effect itself.
        }
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
