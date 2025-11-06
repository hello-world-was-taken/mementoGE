#pragma once

#include "engine/core/GameObject.h"
#include "engine/core/ISystem.h"

#include <vector>

class AttackSystem : public ISystem
{
public:
    void update(std::vector<GameObject> &gameObjects) override;

private:
    void performAttack(GameObject &enemy, GameObject &player);
};
