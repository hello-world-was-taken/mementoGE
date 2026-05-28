#pragma once

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

using ShaderUniformValue = std::variant<int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat4>;

struct ShaderEffectRequest
{
    std::string vertexShaderPath;
    std::string fragmentShaderPath;

    // TODO: update serialization logic to support this
    std::unordered_map<std::string, ShaderUniformValue> uniforms;

    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &node);

#ifdef EDITOR_BUILD
    void drawInspector();
#endif
};
