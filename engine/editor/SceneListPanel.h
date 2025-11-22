#pragma once

#include "editor/EditorContext.h"
#include "editor/EditorPanel.h"

#include <string>
#include <vector>

class SceneListPanel : public EditorPanel
{
public:
    SceneListPanel(EditorContext &ctx);
    ~SceneListPanel();

    void draw() override;

private:
    EditorContext &m_ctx;

private:
    void renderSceneListPanel();
};
