#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

struct Transform
{
    glm::vec3 position;
    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::mat4x4 getModelMatrix();

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);
    void drawInspector();
#endif
};