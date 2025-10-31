#pragma once

#include "editor/EditorPanel.h"

class SceneHierarchyPanel : public EditorPanel
{
public:
    SceneHierarchyPanel(EditorContext &ctx);
    ~SceneHierarchyPanel() override = default;

    void draw() override;

private:
    void drawSceneHierarchy();
    void drawGameObjectNode(GameObject &go);
};