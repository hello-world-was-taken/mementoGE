#include "core/components/Transform.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Sprite.h"

#include "core/MovementSystem.h"
#include "core/GameObject.h"

#include "util/Time.h"

#include <vector>

void MovementSystem::update(std::vector<GameObject> &gameObjects)
{
    for (auto &go : gameObjects)
    {
        if (!go.hasComponent<MovementComponent>())
            continue;

        auto &movement = go.getComponent<MovementComponent>();
        auto &rb = go.getComponent<RigidBody2D>();
        auto &transform = go.getComponent<Transform>();
        auto &sprite = go.getComponent<Sprite>();

        switch (movement.type)
        {
        case MovementType::Patrol:
            updatePatrol(go, movement);
            break;
        case MovementType::FollowPlayer:
            updateFollow(go, movement);
            break;
        case MovementType::Flying:
            updateFlying(go, movement);
            break;
        default:
            break;
        }
    }
}

void MovementSystem::updatePatrol(GameObject &go, MovementComponent &movement)
{
    if (!go.hasComponent<PatrolMovement>())
        return;
    auto &data = go.getComponent<PatrolMovement>();
    auto &rb = go.getComponent<RigidBody2D>();
    auto &sprite = go.getComponent<Sprite>();

    float delta = Time::deltaTime();
    rb.velocity = {movement.direction * movement.speed, rb.velocity.y};
    data.traveled += movement.speed * delta;

    sprite.flipX = movement.direction < 0;

    if (movement.direction > 0 && data.traveled >= data.rightDistance)
    {
        movement.direction = -1;
        data.traveled = 0.0f;
    }
    else if (movement.direction < 0 && data.traveled >= data.leftDistance)
    {
        movement.direction = 1;
        data.traveled = 0.0f;
    }
}

void MovementSystem::updateFollow(GameObject &go, MovementComponent &movement)
{
    if (!go.hasComponent<FollowMovement>())
        return;
    auto &data = go.getComponent<FollowMovement>();
    auto &rb = go.getComponent<RigidBody2D>();
    auto &transform = go.getComponent<Transform>();

    // TODO: FIX THIS
    GameObject *player = nullptr;

    float playerX = player->getComponent<Transform>().getPosition()->x;
    float enemyX = transform.getPosition()->x;
    float distance = playerX - enemyX;

    if (std::abs(distance) < data.followRange)
    {
        movement.direction = (distance > 0) ? 1 : -1;
        rb.velocity = {movement.direction * movement.speed, rb.velocity.y};
    }
    else
    {
        rb.velocity = {0, rb.velocity.y};
    }
}

void MovementSystem::updateFlying(GameObject &go, MovementComponent &movement)
{
    if (!go.hasComponent<FlyingMovement>())
        return;
    auto &data = go.getComponent<FlyingMovement>();
    auto &transform = go.getComponent<Transform>();
    auto &rb = go.getComponent<RigidBody2D>();

    data.elapsed += Time::deltaTime();
    float yOffset = std::sin(data.elapsed * data.frequency) * data.amplitude;
    transform.translate(movement.direction * movement.speed * Time::deltaTime(), yOffset * Time::deltaTime(), 0);
}