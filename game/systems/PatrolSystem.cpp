#include "engine/core/Animator.h"
#include "engine/core/GameObject.h"
#include "engine/core/components/EnemyState.h"
#include "engine/core/components/Patrol.h"
#include "engine/core/components/RigidBody2D.h"
#include "engine/core/components/Sprite.h"
#include "engine/core/components/Transform.h"
#include "engine/util/Time.h"

#include "game/systems/PatrolSystem.h"

#include <vector>

void PatrolSystem::update(std::vector<GameObject> &gameObjects)
{
    for (auto &go : gameObjects)
    {
        if (!go.hasComponent<EnemyState>())
        {
            continue;
        }
        auto &enemyState = go.getComponent<EnemyState>();
        auto &patrol = go.getComponent<Patrol>();
        auto &rb = go.getComponent<RigidBody2D>();
        auto &sprite = go.getComponent<Sprite>();

        if (enemyState.state == AiState::Dead)
            continue;

        // TODO: get the animation name from patrol struct
        go.getComponent<Animator>().play("walk");

        // Movement speed in pixels/second
        const float delta = Time::deltaTime();

        // Apply velocity
        int direction = patrol.direction ? 1 : -1;
        rb.velocity = {direction * enemyState.moveSpeed, rb.velocity.y};

        // Track how far we've moved this direction
        patrol.currentTraveled += enemyState.moveSpeed * delta;

        // Flip sprite visually
        sprite.flipX = patrol.direction == false;

        // Check if we've traveled far enough
        if (patrol.direction == true && patrol.currentTraveled >= patrol.moveRightDistance)
        {
            patrol.direction = false;
            patrol.currentTraveled = 0.0f;
        }
        else if (patrol.direction == false && patrol.currentTraveled >= patrol.moveLeftDistance)
        {
            patrol.direction = true;
            patrol.currentTraveled = 0.0f;
        }
    }
}
