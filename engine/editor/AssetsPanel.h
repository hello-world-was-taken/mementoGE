#pragma once

#include "editor/EditorContext.h"
#include "editor/EditorPanel.h"

#include <string>
#include <vector>

class AssetsPanel : public EditorPanel
{
public:
    AssetsPanel(EditorContext &ctx);
    ~AssetsPanel();

    void draw() override;

private:
    EditorContext &m_ctx;

private:
    void renderAssetsPanel();
};
