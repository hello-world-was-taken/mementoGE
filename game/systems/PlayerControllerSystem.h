#include "engine/core/GameObject.h"
#include "engine/core/ISystem.h"

#include <entt/entt.hpp>
#include <vector>

class PlayerControllerSystem : public ISystem
{
public:
    void update(entt::registry &registry) override;

private:
    void processInput(const entt::entity &entity, entt::registry &registry);
};