#pragma once
#include "editor/Constants.h" // TODO: core shouldn't depend on editor layer

#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

enum class CameraProjectionType
{
    Orthographic = 0,
    // TODO: perspective could be added later
};

struct Camera
{
    glm::mat4 projection{1.0f};
    glm::mat4 view{1.0f};

    CameraProjectionType projectionType = CameraProjectionType::Orthographic;

    // TODO: Zoom influences orthographic size or editor navigation
    // So, does it make sense for gameplay?
    float zoom = 1.0f;
    float minZoom = 0.1f;
    float maxZoom = 10.0f;

    // True = main gameplay camera.
    bool primary = true;

    // TODO: I think it might be better if we started at some random number
    // and then adjust it after adding the component
    float logicalWidth = LOGICAL_WIDTH;
    float logicalHeight = LOGICAL_HEIGHT;

    // Near & far planes
    float nearClip = -100.0f;
    float farClip = 100.0f;

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out) const;
    void deserialize(const YAML::Node &in);
    void drawInspector();
#endif
};
