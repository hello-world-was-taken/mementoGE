#pragma once

#include "engine/core/GameObject.h"
#include "engine/core/ISystem.h"

#include <entt/entt.hpp>

class EnemySystem : ISystem
{
public:
    void update(std::vector<GameObject> &gameObjects) override;

private:
    void updateAnimation();
    void move();
};