#include "game/systems/EnemyAiSystem.h"

#include "engine/core/GameObject.h"
#include "engine/core/components/EnemyState.h"
#include "engine/core/components/Sensor2D.h"

#include <iostream>

void EnemyAiSystem::update(std::vector<GameObject> &gameObjects)
{
    for (auto &enemy : gameObjects)
    {
        if (enemy.getTag() != "Enemy")
            continue;

        if (!enemy.hasComponent<EnemyState>() || !enemy.hasComponent<Sensor2D>())
            continue;

        auto &ai = enemy.getComponent<EnemyState>();
        auto &sensor = enemy.getComponent<Sensor2D>();

        // Check if player overlaps this sensor
        bool playerDetected = isPlayerInRange(sensor);

        AiState prevState = ai.state;
        ai.state = playerDetected ? AiState::Attack : AiState::Patrol;

        if (ai.state != prevState)
        {
            std::cout << "[EnemyStateSystem] " << enemy.getTag() << " switched to "
                      << (ai.state == AiState::Attack ? "Attack" : "Patrol") << " state\n";
        }
    }
}

bool EnemyAiSystem::isPlayerInRange(const Sensor2D &sensor) const
{
    for (auto *objPtr : sensor.overlappingObjects)
    {
        GameObject *obj = static_cast<GameObject *>(objPtr);
        if (obj && obj->getTag() == "Player")
        {
            return true;
        }
    }
    return false;
}
