#pragma once

#include "core/EditorContext.h"
#include "core/EditorPanel.h"
#include "core/TexturePanel.h"

class PropertiesPanel : public EditorPanel
{
public:
    PropertiesPanel(EditorContext &ctx, TexturePanel &texturePanel);
    ~PropertiesPanel();

    void draw() override;

private:
    EditorContext &m_ctx;
    TexturePanel &m_texturePanel;

private:
    void renderPropertiesPanel();
};