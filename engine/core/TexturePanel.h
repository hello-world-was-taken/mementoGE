#pragma once

#include "core/EditorPanel.h"
#include "core/EditorContext.h"

class TexturePanel : public EditorPanel
{
public:
    TexturePanel(EditorContext &ctx);
    ~TexturePanel();

    void draw();

private:
    EditorContext &m_ctx;

private:
    void renderTextureAssetsListPanel();
    void copyTextureToAssets();
    void renderSelectedTexSheetPanel(bool isInModal);
};