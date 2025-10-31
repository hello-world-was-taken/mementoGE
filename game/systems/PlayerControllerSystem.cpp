#include "game/systems/PlayerControllerSystem.h"

#include "engine/core/Animator.h"
#include "engine/core/EventHandler.h"
#include "engine/core/components/RigidBody2D.h"
#include "engine/core/components/Sprite.h"
#include "engine/util/Time.h"

#include <algorithm>

void PlayerControllerSystem::update(std::vector<GameObject> &gameObjects)
{
    // TODO: Improve this. Searching for the player on every update.
    auto it = std::find_if(gameObjects.begin(), gameObjects.end(),
        [](GameObject &go)
        {
            return go.getTag() == "Player";
        });
    if (it == gameObjects.end())
    {
        return;
    }
    processInput(it[0]);
}

void PlayerControllerSystem::processInput(GameObject &player)
{
    auto *eventHandler = EventHandler::instance();
    if (eventHandler->hasActiveEvent())
    {
        Event e = eventHandler->getCurrentEvent();

        if (e.getEventType() == EventType::Key || e.getEventType() == EventType::KeyRepeat)
        {
            KeyType keyType = e.getKeyType();

            if (keyType == KeyType::RightArrow)
            {
                player.getComponent<RigidBody2D>().velocity += glm::vec2{50.0f * Time::deltaTime(), 0.0f};
                player.getComponent<Sprite>().flipX = true;
                player.getComponent<Animator>().play("idle");
            }
            else if (keyType == KeyType::LeftArrow)
            {
                player.getComponent<RigidBody2D>().velocity += glm::vec2{-50.0f * Time::deltaTime(), 0.0f};
                player.getComponent<Sprite>().flipX = false;
                player.getComponent<Animator>().play("idle");
            }
        }
    }
    else
    {
        player.getComponent<RigidBody2D>().velocity = glm::vec2{0.0f, 0.0f};
        player.getComponent<Animator>().play("idle");
    }
}