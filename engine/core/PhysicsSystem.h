#pragma once

#include "core/ISystem.h"

class PhysicsSystem : public ISystem
{
public:
    void update(SystemContext &context) override;
    int executionOrder() const override
    {
        return 300;
    }
};
