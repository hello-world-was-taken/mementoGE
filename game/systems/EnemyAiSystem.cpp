#include "game/systems/EnemyAiSystem.h"

#include "engine/core/GameObject.h"
#include "engine/core/components/EnemyState.h"
#include "engine/core/components/EntityInfo.h"
#include "engine/core/components/Sensor2D.h"
#include "engine/core/components/Transform.h"

#include <algorithm>
#include <entt/entt.hpp>
#include <iostream>
#include <glm/glm.hpp>

inline const char *toString(AiState state)
{
    switch (state)
    {
    case AiState::Idle:
        return "Idle";
    case AiState::Patrol:
        return "Patrol";
    case AiState::Chase:
        return "Chase";
    case AiState::Fly:
        return "Fly";
    case AiState::Attack:
        return "Attack";
    case AiState::Dead:
        return "Dead";
    default:
        return "Unknown";
    }
}

void EnemyAiSystem::update(entt::registry &registry)
{
    auto view = registry.view<EntityInfo, EnemyState, Sensor2D, RigidBody2D>();
    for (entt::entity entity : view)
    {
        EntityInfo &entityInfo = registry.get<EntityInfo>(entity);
        EnemyState &enemyState = registry.get<EnemyState>(entity);
        Sensor2D &sensor = registry.get<Sensor2D>(entity);
        RigidBody2D &rb = registry.get<RigidBody2D>(entity);
        Transform &transform = registry.get<Transform>(entity);

        // Check if player overlaps this sensor
        entt::entity player = findPlayer(sensor, registry);
        if (player == entt::null)
        {
            if (enemyState.state != AiState::Patrol && enemyState.state != AiState::Return)
            {
                enemyState.state = AiState::Return;
            }
            continue;
        }

        Transform &playerTransform = registry.get<Transform>(player);

        float distance = glm::distance(transform.position, playerTransform.position);
        bool playerDetected = distance <= sensor.size[0]; // TODO: using only x axis for now

        AiState prev = enemyState.state;

        switch (enemyState.state)
        {
        case AiState::Patrol:
            if (playerDetected)
            {
                enemyState.state = AiState::Chase;
            }
            break;

        case AiState::Chase:
            if (!playerDetected)
            {
                enemyState.state = AiState::Return;
            }
            else if (distance <= enemyState.attackRange)
            {
                enemyState.state = AiState::Attack;
            }
            break;

        case AiState::Attack:
            if (!playerDetected)
            {
                enemyState.state = AiState::Return;
            }
            else if (distance > enemyState.attackRange)
            {
                enemyState.state = AiState::Chase;
            }
            break;

        case AiState::Return:
            if (glm::distance(transform.position, enemyState.startPosition) < 1.0f)
            {
                enemyState.state = AiState::Patrol;
            }
            break;

        default:
            break;
        }

        if (enemyState.state != prev)
        {
            std::cout << "[EnemyAI] " << entityInfo.tag << " → " << toString(enemyState.state) << "\n";
        }
    }
}

entt::entity EnemyAiSystem::findPlayer(const Sensor2D &sensor, entt::registry &registry) const
{
    for (entt::entity entity : sensor.overlappingObjects)
    {
        EntityInfo &entityInfo = registry.get<EntityInfo>(entity);
        if (entityInfo.tag == "Player")
        {
            return entity;
        }
    }

    return entt::null;
}
