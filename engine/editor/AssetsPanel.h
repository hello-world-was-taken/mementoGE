#pragma once

#include "editor/EditorContext.h"
#include "editor/EditorPanel.h"

#include <string>
#include <vector>

class AssetsPanel : public EditorPanel
{
public:
    AssetsPanel(EditorContext &ctx);
    ~AssetsPanel() override;

    void draw() override;

private:
    void buildAssetsDockspace();

    void drawFoldersPanel();
    void drawContentPanel();
    void drawSpritePanel();
    void drawAnimationPanel();
    void drawBoundTextures();

private:
    bool m_rebuildDock = true; // rebuild dock on on first render
    EditorContext &m_ctx;
};
