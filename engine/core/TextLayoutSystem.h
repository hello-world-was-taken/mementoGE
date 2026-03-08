#pragma once

#include "core/ISystem.h"

// System that positions Text components according to a TextAnchor
// component, anchoring the text within the owning game object's
// rectangle (using EntityInfo width/height) instead of the global
// logical viewport.
class TextLayoutSystem : public ISystem
{
public:
    TextLayoutSystem() = default;
    ~TextLayoutSystem() override = default;

    void update(entt::registry &registry) override;
};
