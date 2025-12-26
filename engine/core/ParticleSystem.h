#pragma once

#include "core/ISystem.h"

#include <entt/entt.hpp>

class ParticleSystem : public ISystem
{
public:
    void update(entt::registry &registry) override;
};
