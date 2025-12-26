#pragma once

#include "opengl/Texture.h"

#include <glm/glm.hpp>
#include <memory>

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

struct Particle
{
    glm::vec3 pos;
    glm::vec3 vel;
    float life;    // remaining lifetime
    float maxLife; // initial lifetime. Required since each particle is initialized with random maxLife
    float size;
    glm::vec4 color;
};

struct ParticleEmitter
{
    std::vector<Particle> particles;

    // spawn params
    float rate = 50.0f; // particles per second
    float accumulator = 0.0f;
    float minLife = 0.5f;
    float maxLife = 2.0f;
    float minSize = 0.1f;
    float maxSize = 0.5f;

    glm::vec3 spawnOffset = {0.0f, 0.0f, 0.0f};
    glm::vec3 baseVelocity = {0.0f, 1.0f, 0.0f};
    glm::vec4 baseColor = {1, 1, 1, 1};

    // TODO: for now lets just have color
    // later, we'll using Sprite and add drag and drop functionality for it as well
    // std::shared_ptr<Texture> texture; // fire / smoke texture

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &out);
    void drawInspector();
#endif
};
