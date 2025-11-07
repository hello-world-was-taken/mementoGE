#pragma once

#include <glm/glm.hpp>

#ifdef EDITOR_BUILD
#include <imgui.h>
#include <yaml-cpp/yaml.h>
#endif

enum class AiState
{
    Idle,
    Patrol,
    Chase,
    Fly,
    Attack,
    Return,
    Dead,
};

enum class EnemyType
{
    Minion,
    Elite,
    Boss
};

struct EnemyState
{
    EnemyType type = EnemyType::Minion;
    AiState state = AiState::Patrol;

    float health = 100.0f;
    float moveSpeed = 50.0f;
    float attackPower = 10.0f;

    float attackCooldown = 1.0f;

    float detectionRadius = 150.0f;
    float attackRange = 30.0f;

    // doesn't need to be serialized as at the start,
    // it'll be the same as the entity's transfomr
    glm::vec3 startPosition;

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &out);
    void drawInspector();
#endif
};
