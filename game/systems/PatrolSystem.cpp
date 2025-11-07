#include "engine/core/Animator.h"
#include "engine/core/GameObject.h"
#include "engine/core/components/EnemyState.h"
#include "engine/core/components/EntityInfo.h"
#include "engine/core/components/Patrol.h"
#include "engine/core/components/RigidBody2D.h"
#include "engine/core/components/Sprite.h"
#include "engine/core/components/Transform.h"
#include "engine/util/Time.h"

#include "game/systems/PatrolSystem.h"

#include <entt/entt.hpp>
#include <vector>

void PatrolSystem::update(entt::registry &registry)
{
    auto view = registry.view<EnemyState, Patrol, Sensor2D, RigidBody2D, Sprite, Animator, Transform>();
    for (entt::entity entity : view)
    {
        EnemyState &enemyState = registry.get<EnemyState>(entity);
        Transform &transform = registry.get<Transform>(entity);
        Patrol &patrol = registry.get<Patrol>(entity);
        Sensor2D &sensor = registry.get<Sensor2D>(entity);
        RigidBody2D &rb = registry.get<RigidBody2D>(entity);
        Sprite &sprite = registry.get<Sprite>(entity);
        Animator &animator = registry.get<Animator>(entity);

        if (enemyState.state == AiState::Dead)
            continue;

        // TODO: get the animation name from patrol struct
        const float delta = Time::deltaTime();

        // ============================================================
        // 1. CHASE BEHAVIOR
        // ============================================================
        if (enemyState.state == AiState::Chase)
        {
            // Play run / chase animation
            animator.play("run");

            // Find player in sensor overlaps
            entt::entity playerEnt = entt::null;
            for (auto e : sensor.overlappingObjects)
            {
                if (registry.valid(e))
                {
                    auto &info = registry.get<EntityInfo>(e);
                    if (info.tag == "Player")
                    {
                        playerEnt = e;
                        break;
                    }
                }
            }

            if (playerEnt != entt::null)
            {
                auto &playerTransform = registry.get<Transform>(playerEnt);

                glm::vec3 dir = playerTransform.position - transform.position;

                // Normalize only if length > 0
                if (glm::length(dir) > 0.001f)
                    dir = glm::normalize(dir);

                rb.velocity = {dir.x * enemyState.moveSpeed, rb.velocity.y};

                // Flip sprite to face player
                sprite.flipX = (dir.x < 0.0f);
            }

            continue; // skip patrol + return behavior
        }

        // ========== RETURN BEHAVIOR ==========
        if (enemyState.state == AiState::Return)
        {
            animator.play("walk");

            float toHomeX = abs(enemyState.startPosition.x) - abs(transform.position.x);
            std::cout << "toHomeX: " << (toHomeX < 1.0f) << std::endl;

            // Already at home → resume patrol
            if (toHomeX < 1.0f)
            {
                enemyState.state = AiState::Patrol;
                patrol.currentTraveled = 0.0f; // reset patrol progress
                continue;
            }

            int direction = toHomeX > 0 ? 1 : -1;

            // Move toward start position
            rb.velocity = {direction * enemyState.moveSpeed, rb.velocity.y};

            // Flip sprite based on horizontal movement
            sprite.flipX = (direction == -1);

            continue; // Skip Patrol behavior
        }

        // =========== PATROL BEHAVIOR ============
        if (enemyState.state != AiState::Patrol)
            continue;

        animator.play("walk");
        int direction = patrol.direction ? 1 : -1;
        rb.velocity = {direction * enemyState.moveSpeed, rb.velocity.y};

        // Track how far we've moved this direction
        patrol.currentTraveled += enemyState.moveSpeed * delta;

        // Flip sprite visually
        sprite.flipX = patrol.direction == false;

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
