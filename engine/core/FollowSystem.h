#pragma once

#include "core/ISystem.h"

// FollowSystem resolves EntityRef links and chooses a follow strategy based on
// whether the follower and target are physics-driven.
class FollowSystem : public ISystem
{
public:
    void update(SystemContext &context) override;
};
