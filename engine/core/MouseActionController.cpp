#include <iostream>

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

    // the -16 is to center the object with the cursor. The overal size of the object is 32
    activeObject->getComponent<Transform>().setPosition(mouseWorldPos.x - 16, mouseWorldPos.y - 16, 0);
    if (mouse->isLeftMouseClicked())
    {
        activeObject = nullptr;
    }
}
