#pragma once

#include "core/SystemContext.h"
#include "core/SystemDescriptor.h"

// TODO: we can avoid the vtable lookup by directly
// passing the function ptrs. Would it be worth it in this case?
class ISystem
{
public:
    virtual ~ISystem() = default;
    virtual void update(SystemContext &context) = 0;
    virtual SystemDescriptor descriptor() const = 0;
};
