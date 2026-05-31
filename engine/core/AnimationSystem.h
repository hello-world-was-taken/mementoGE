#pragma once

#include "core/ISystem.h"

class AnimationSystem : public ISystem
{
public:
    void update(SystemContext &context) override;
    SystemDescriptor descriptor() const override;
    int executionOrder() const override
    {
        return 200;
    }
};
