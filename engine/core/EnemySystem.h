#pragma once

#include "core/EnemyState.h"
#include "core/GameObject.h"

#include <entt/entt.hpp>

struct EnemyAiState
{
    EnemyState current = EnemyState::Idle;
    float currentTraveled = 0.0f;
    float moveLeftDistance = 64.0f;
    float moveRightDistance = 64.0f;
    int direction = 1; // 1 (right) - -1 (left)
};

class EnemySystem
{
public:
    void update(std::vector<GameObject> &gameObjects);

private:
    void updateAnimation();
    void move();
};