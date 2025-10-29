#include "engine/core/components/RigidBody2D.h"
#include "engine/core/components/EnemyStats.h"
#include "engine/core/components/Transform.h"
#include "engine/core/components/Sprite.h"
#include "engine/core/GameObject.h"

#include "EnemySystem.h"

#include "engine/util/Time.h"

void EnemySystem::update(std::vector<GameObject> &gameObjects)
{
    // auto &registry = Scene::getActive()->getRegistry(); // adjust to your engine’s API

    // auto view = registry.view<EnemyAiState, Transform, RigidBody2D, Sprite>();

    for (auto &go : gameObjects)
    {
        if (!go.hasComponent<EnemyStats>())
        {
            continue;
        }
        auto &aiStat = go.getComponent<EnemyStats>();
        auto &currState = go.getComponent<CurrentAiState>();
        auto &transform = go.getComponent<Transform>();
        auto &rb = go.getComponent<RigidBody2D>();
        auto &sprite = go.getComponent<Sprite>();

        if (currState.state == AiState::Dead)
            continue;

        // Movement speed in pixels/second
        const float moveSpeed = 50.0f;
        const float delta = Time::deltaTime();

        // Apply velocity
        rb.velocity = {currState.direction * moveSpeed, rb.velocity.y};

        // Track how far we've moved this direction
        currState.currentTraveled += moveSpeed * delta;

        // Flip sprite visually
        sprite.flipX = currState.direction < 0;

        // Check if we've traveled far enough
        if (currState.direction > 0 && currState.currentTraveled >= aiStat.moveRightDistance)
        {
            currState.direction = -1;
            currState.currentTraveled = 0.0f;
        }
        else if (currState.direction < 0 && currState.currentTraveled >= aiStat.moveLeftDistance)
        {
            currState.direction = 1;
            currState.currentTraveled = 0.0f;
        }
    }
}
