#pragma once

#include "engine/core/GameObject.h"
#include "engine/core/ISystem.h"

#include "engine/core/components/EnemyState.h"
#include "engine/core/components/Sensor2D.h"

#include <algorithm>
#include <entt/entt.hpp>
#include <vector>

class EnemyAiSystem : public ISystem
{
public:
    void update(entt::registry &registry) override;

private:
    entt::entity findPlayer(const Sensor2D &sensor, entt::registry &registry) const;
};
