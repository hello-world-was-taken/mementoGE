#pragma once

#include "engine/core/GameObject.h"
#include "engine/core/ISystem.h"

#include <entt/entt.hpp>
#include <vector>

class AttackSystem : public ISystem
{
public:
    void update(entt::registry &registry) override;

private:
    void performAttack(GameObject &enemy, GameObject &player);
};
