#pragma once

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
    Dead
};

enum class EnemyType
{
    Minion,
    Elite,
    Boss
};

struct CurrentAiState
{
    AiState state = AiState::Idle;

    int direction = 1;
    float currentTraveled = 0.0f;
    float timeSinceLastAttack = 0.0f;
};

struct EnemyStats
{
    EnemyType type = EnemyType::Minion;

    float health = 100.0f;
    float moveSpeed = 50.0f;
    float attackPower = 10.0f;

    float attackCooldown = 1.0f;

    float detectionRadius = 150.0f;
    float attackRange = 30.0f;

    float moveLeftDistance = 64.0f;
    float moveRightDistance = 64.0f;

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &out);
    void drawInspector();
#endif
};
