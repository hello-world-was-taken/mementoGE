#include "game/systems/PlayerControllerSystem.h"

#include "engine/core/Animator.h"
#include "engine/core/EventHandler.h"
#include "engine/core/components/EntityInfo.h"
#include "engine/core/components/RigidBody2D.h"
#include "engine/core/components/Sprite.h"
#include "engine/util/Time.h"

#include <algorithm>
#include <entt/entt.hpp>

void PlayerControllerSystem::update(entt::registry &registry)
{
    auto view = registry.view<EntityInfo>();
    auto it = std::find_if(view.begin(), view.end(),
        [&](const entt::entity &entity)
        {
            EntityInfo &entityInfo = registry.get<EntityInfo>(entity);
            return entityInfo.tag == "Player";
        });

    if (it == view.end())
    {
        std::cout << "Player not found" << std::endl;
        return;
    }
    processInput(*it, registry);
}

void PlayerControllerSystem::processInput(const entt::entity &entity, entt::registry &registry)
{
    Sprite &sprite = registry.get<Sprite>(entity);
    Animator &animator = registry.get<Animator>(entity);
    RigidBody2D &rb = registry.get<RigidBody2D>(entity);

    auto *eventHandler = EventHandler::instance();
    if (eventHandler->hasActiveEvent())
    {
        Event e = eventHandler->getCurrentEvent();

        if (e.getEventType() == EventType::Key || e.getEventType() == EventType::KeyRepeat)
        {
            KeyType keyType = e.getKeyType();

            if (keyType == KeyType::RightArrow)
            {
                rb.velocity += glm::vec2{50.0f * Time::deltaTime(), 0.0f};
                sprite.flipX = true;
                animator.play("run");
            }
            else if (keyType == KeyType::LeftArrow)
            {
                rb.velocity += glm::vec2{-50.0f * Time::deltaTime(), 0.0f};
                sprite.flipX = false;
                animator.play("run");
            }
        }
    }
    else
    {
        rb.velocity = glm::vec2{0.0f, 0.0f};
        animator.play("idle");
    }
}