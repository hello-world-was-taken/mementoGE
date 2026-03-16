#pragma once

#include <cstdint>
#include <string>
#include <vector>

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

enum class RenderLayerType
{
    Background = 0,
    World = 1,
    Foreground = 2,
    UI = 3,
    Debug = 4
};

static std::vector<RenderLayerType> RENDER_LAYER_ORDER = {RenderLayerType::Background,
    RenderLayerType::World,
    RenderLayerType::Foreground,
    RenderLayerType::UI,
    RenderLayerType::Debug};

struct RenderLayer
{
    RenderLayerType layer = RenderLayerType::World; // default
    int order = 0;                                  // fine sorting inside the layer
    uint32_t mask = 0xffffffff;                     // camera visibility mask

    static std::string renderLayerTypeToString(RenderLayerType type);
    static RenderLayerType renderLayerTypeFromString(const std::string &s);

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out) const;
    void deserialize(const YAML::Node &in);
    void drawInspector();
#endif
};
