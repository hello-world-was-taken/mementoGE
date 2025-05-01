#include <iostream>
#include <cmath>

#include "core/MouseActionController.h"
#include "core/Transform.h"

MouseActionController::MouseActionController()
    : activeObject(nullptr) {}

void MouseActionController::SetActiveObject(GameObject &object)
{
    activeObject = &object;
}

void MouseActionController::Update(std::shared_ptr<Camera> camera)
{
    if (!activeObject)
        return;

    MouseListener *mouse = MouseListener::getListener();
    glm::vec2 mouseWorldPos = mouse->getWorldCoordinates(camera);

    // Assume grid size is equal to the object's width.
    // TODO: is there a scenario where we would want to change this?
    float gridSize = static_cast<float>(activeObject->getWidth());

    // Snap position: round down to nearest multiple of gridSize
    float snappedX = std::floor(mouseWorldPos.x / gridSize) * gridSize;
    float snappedY = std::floor(mouseWorldPos.y / gridSize) * gridSize;

    activeObject->getComponent<Transform>().setPosition(snappedX, snappedY, 0.0f);

    if (mouse->isLeftMouseClicked())
    {
        activeObject = nullptr;
    }
}
