#pragma once

#include "core/GameObject.h"

#include <vector>

// TODO: we can avoid the vtable lookup by directly
// passing the function ptrs. Would it be worth it in this case?
class ISystem
{
public:
    virtual ~ISystem() = default;
    virtual void update(std::vector<GameObject> &gameObjects) = 0;
};