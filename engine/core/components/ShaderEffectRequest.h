#pragma once

#include <string>

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

struct ShaderEffectRequest
{
    std::string vertexShaderPath;
    std::string fragmentShaderPath;

    // Optional: You can add more fields here, such as uniforms or other shader parameters.

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &node);
    void drawInspector();
#endif
};