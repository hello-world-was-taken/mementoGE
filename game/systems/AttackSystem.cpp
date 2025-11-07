#include "game/systems/AttackSystem.h"

#include "engine/core/Animator.h"
#include "engine/core/components/EnemyState.h"
#include "engine/core/components/EntityInfo.h"
#include "engine/core/components/RigidBody2D.h"
#include "engine/core/components/Sensor2D.h"
#include "engine/core/components/Sprite.h"
#include "engine/core/components/Transform.h"
#include "engine/util/Time.h"

#include <algorithm>
#include <entt/entt.hpp>
#include <iostream>

void AttackSystem::update(entt::registry &registry)
{
    auto view = registry.view<EnemyState, EnemyState, Sensor2D, Animator, RigidBody2D>();
    for (entt::entity entity : view)
    {
        EnemyState &state = registry.get<EnemyState>(entity);
        Sensor2D &sensor = registry.get<Sensor2D>(entity);
        Animator &animator = registry.get<Animator>(entity);
        RigidBody2D &rb = registry.get<RigidBody2D>(entity);

        if (state.state != AiState::Attack || state.state == AiState::Dead)
            continue;

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
    EntityInfo &enemyEntityInfo = enemy.getComponent<EntityInfo>();
    EntityInfo &playerEntityInfo = player.getComponent<EntityInfo>();
    std::cout << "[AttackSystem] " << enemyEntityInfo.tag << " attacked player: " << playerEntityInfo.tag << std::endl;

    // Example: play animation, trigger hitbox, or reduce health
    // player.getComponent<Health>().applyDamage(10);
}
