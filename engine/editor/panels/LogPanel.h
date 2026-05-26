#pragma once

#include "editor/EditorPanel.h"

#include <array>

class LogPanel : public EditorPanel
{
public:
    explicit LogPanel(EditorContext &ctx);
    ~LogPanel() override = default;

    void draw() override;

private:
    bool m_showDebug = true;
    bool m_showInfo = true;
    bool m_showWarnings = true;
    bool m_showErrors = true;
    bool m_autoScroll = true;
    std::array<char, 128> m_filter{};
};
