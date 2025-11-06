#include "game/systems/AttackSystem.h"

#include "engine/core/Animator.h"
#include "engine/core/components/EnemyState.h"
#include "engine/core/components/RigidBody2D.h"
#include "engine/core/components/Sensor2D.h"
#include "engine/core/components/Sprite.h"
#include "engine/core/components/Transform.h"
#include "engine/util/Time.h"

#include <algorithm>
#include <iostream>

void AttackSystem::update(std::vector<GameObject> &gameObjects)
{
    for (auto &enemy : gameObjects)
    {
        if (!enemy.hasComponent<EnemyState>())
            continue;

        auto &state = enemy.getComponent<EnemyState>();
        if (state.state != AiState::Attack || state.state == AiState::Dead)
            continue;

        if (!enemy.hasComponent<Sensor2D>())
            continue;

        auto &sensor = enemy.getComponent<Sensor2D>();
        auto &animator = enemy.getComponent<Animator>();
        auto &rb = enemy.getComponent<RigidBody2D>();

        // Stop horizontal movement when attacking
        rb.velocity.x = 0.0f;

        // Play attack animation
        animator.play("attack");

        // Handle cooldown timer
        // state.currentCooldown -= Time::deltaTime();
        // if (state.currentCooldown > 0.0f)
        //     continue;

        // // Find the player in range
        // for (auto *objPtr : sensor.overlappingObjects)
        // {
        //     GameObject *target = static_cast<GameObject *>(objPtr);
        //     if (target && target->getTag() == "Player")
        //     {
        //         performAttack(enemy, *target);
        //         state.currentCooldown = state.attackCooldown;
        //         break;
        //     }
        // }
    }
}

void AttackSystem::performAttack(GameObject &enemy, GameObject &player)
{
    // This is where you’d apply damage, knockback, etc.
    std::cout << "[AttackSystem] " << enemy.getTag() << " attacked player: " << player.getTag() << std::endl;

    // Example: play animation, trigger hitbox, or reduce health
    // player.getComponent<Health>().applyDamage(10);
}
