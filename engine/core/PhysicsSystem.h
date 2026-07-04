#pragma once

#include "core/ISystem.h"

class PhysicsSystem : public ISystem
{
public:
    void update(SystemContext &context) override;
    SystemDescriptor descriptor() const override;
};
