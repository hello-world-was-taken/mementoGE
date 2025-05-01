#pragma once

#include <memory>

#include "core/MouseListener.h"
#include "core/GameObject.h"

class MouseActionController
{
public:
    MouseActionController();

    void SetActiveObject(GameObject& object);
    // camera required for world-space conversion
    // game object required for selection detection
    void Update(std::shared_ptr<Camera> camera, std::vector<GameObject> &gameObjects);

private:
    GameObject* activeObject;
};
