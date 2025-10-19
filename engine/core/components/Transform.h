#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

struct Transform
{
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;

    glm::mat4x4 getModelMatrix();

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);
    void drawInspector();
#endif
};