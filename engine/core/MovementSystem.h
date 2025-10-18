#pragma once
#include "core/GameObject.h"

enum class MovementType
{
    None,
    Patrol,
    FollowPlayer,
    Flying,
    Jumping,
};

struct MovementComponent
{
    MovementType type = MovementType::None;
    float speed = 50.0f;
    int direction = 1; // 1 = right, -1 = left
};

struct PatrolMovement
{
    float leftDistance = 64.0f;
    float rightDistance = 64.0f;
    float traveled = 0.0f;
};

struct FollowMovement
{
    float followRange = 200.0f;
};

struct FlyingMovement
{
    float amplitude = 10.0f;
    float frequency = 2.0f;
    float elapsed = 0.0f;
};

class MovementSystem
{
public:
    void update(std::vector<GameObject> &gameObjects);

private:
    void updatePatrol(GameObject &go, MovementComponent &movement);
    void updateFollow(GameObject &go, MovementComponent &movement);
    void updateFlying(GameObject &go, MovementComponent &movement);
};
