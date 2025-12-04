#pragma once

#include "core/components/RenderLayer.h"

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

struct PostProcessSettings
{
    bool enabled = true;

    struct BlurSettings
    {
        bool enabled = false;
        float radius = 2.0f;
        std::vector<RenderLayerType> targetLayers;
    } blur;

    struct BloomSettings
    {
        bool enabled = false;
        float threshold = 1.0f;
        std::vector<RenderLayerType> targetLayers;
    } bloom;

    struct ColorGradingSettings
    {
        bool enabled = false;
        float saturation = 1.0f;
        std::vector<RenderLayerType> targetLayers;
    } grade;

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out) const;
    void deserialize(const YAML::Node &in);
    void drawInspector();
#endif
};
