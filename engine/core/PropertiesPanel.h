#pragma once

#include "core/EditorContext.h"
#include "core/EditorPanel.h"

class PropertiesPanel : public EditorPanel
{
public:
    PropertiesPanel(EditorContext &ctx);
    ~PropertiesPanel();

    void draw() override;

private:
    EditorContext &m_ctx;

private:
    void renderPropertiesPanel();
    void renderSelectedTexSheetPanel(bool isInModal);
};