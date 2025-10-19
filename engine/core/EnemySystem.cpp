#include "core/components/Transform.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Sprite.h"

#include "core/EnemySystem.h"
#include "core/GameObject.h"

#include "util/Time.h"

void EnemySystem::update(std::vector<GameObject> &gameObjects)
{
    // auto &registry = Scene::getActive()->getRegistry(); // adjust to your engine’s API

    // auto view = registry.view<EnemyAiState, Transform, RigidBody2D, Sprite>();

    for (auto &go : gameObjects)
    {
        if (!go.hasComponent<EnemyAiState>())
        {
            continue;
        }
        auto &ai = go.getComponent<EnemyAiState>();
        auto &transform = go.getComponent<Transform>();
        auto &rb = go.getComponent<RigidBody2D>();
        auto &sprite = go.getComponent<Sprite>();

        if (ai.current == EnemyState::Dead)
            continue;

        // Movement speed in pixels/second
        const float moveSpeed = 50.0f;
        const float delta = Time::deltaTime();

        // Apply velocity
        rb.velocity = {ai.direction * moveSpeed, rb.velocity.y};

        // Track how far we've moved this direction
        ai.currentTraveled += moveSpeed * delta;

        // Flip sprite visually
        sprite.flipX = ai.direction < 0;

        // Check if we've traveled far enough
        if (ai.direction > 0 && ai.currentTraveled >= ai.moveRightDistance)
        {
            ai.direction = -1;
            ai.currentTraveled = 0.0f;
        }
        else if (ai.direction < 0 && ai.currentTraveled >= ai.moveLeftDistance)
        {
            ai.direction = 1;
            ai.currentTraveled = 0.0f;
        }
    }
}
