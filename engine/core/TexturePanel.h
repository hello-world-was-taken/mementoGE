#pragma once
#include "core/EditorPanel.h"
#include "core/EditorContext.h"
#include "core/Sprite.h"

#include <functional>

class TexturePanel : public EditorPanel
{
public:
    TexturePanel(EditorContext &ctx);
    ~TexturePanel();

    void draw();
    // TODO: should we make TexturePanel a friend of PropertiesPanel?
    void renderSelectedTexSheetPanel(bool isInModal, std::function<void(Sprite &sprite)> onClick = nullptr);

private:
    EditorContext &m_ctx;

private:
    void renderTextureAssetsListPanel();
    void renderAnimationPanel();
    void copyTextureToAssets();
};