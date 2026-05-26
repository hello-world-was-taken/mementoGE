#pragma once

#include "core/ISystem.h"

class AnimationSystem : public ISystem
{
public:
    void update(SystemContext &context) override;
    int executionOrder() const override
    {
        return 200;
    }
};
