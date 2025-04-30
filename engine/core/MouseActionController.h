#pragma once

#include <memory>

#include "core/MouseListener.h"
#include "core/GameObject.h"

class MouseActionController
{
public:
    MouseActionController();

    void SetActiveObject(GameObject& object);
    void Update(std::shared_ptr<Camera> camera); // camera required for world-space conversion

private:
    GameObject* activeObject;
};
