#include "engine/core/GameObject.h"
#include "engine/core/ISystem.h"

#include <vector>

class PlayerControllerSystem : public ISystem
{
public:
    void update(std::vector<GameObject> &gameObjects) override;

    void processInput(GameObject &player);
};