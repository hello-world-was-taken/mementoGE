#pragma once

#include "editor/EditorPanel.h"

class SystemsPanel : public EditorPanel
{
public:
    explicit SystemsPanel(EditorContext &ctx);
    ~SystemsPanel() override = default;

    void draw() override;
};
