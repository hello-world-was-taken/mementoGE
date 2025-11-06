#pragma once

#include "engine/core/GameObject.h"
#include "engine/core/ISystem.h"

#include "engine/core/components/EnemyState.h"
#include "engine/core/components/Sensor2D.h"

#include <algorithm>
#include <vector>

class EnemyAiSystem : public ISystem
{
public:
    void update(std::vector<GameObject> &gameObjects) override;

private:
    bool isPlayerInRange(const Sensor2D &sensor) const;
};
