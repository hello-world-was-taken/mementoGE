#pragma once

#include "core/components/Sprite.h"

#include "editor/EditorPanel.h"
#include "editor/EditorContext.h"

#include <functional>

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
};