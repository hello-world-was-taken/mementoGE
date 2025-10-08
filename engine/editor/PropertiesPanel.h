#pragma once

#include "editor/TexturePanel.h"
#include "editor/EditorContext.h"
#include "editor/EditorPanel.h"

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
    void renderPropertiesInWindow();
    void renderPropertiesInPopup();

    void drawSize(GameObject *go);
    void drawTransform(GameObject *go);
    void drawLayer(GameObject *go);
    void drawSpriteSettings(GameObject *go);
    void drawAddComponentCombo(GameObject *go);
    void drawRigidBodySettings(GameObject *go);
    void drawBoxColliderSettings(GameObject *go);
    void drawAnimatorSettings(GameObject *go);
    void drawPopups();
};