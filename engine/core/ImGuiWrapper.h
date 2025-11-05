#pragma once

#include "core/GLIncludes.h"
#include "core/Window.h"

#include <functional>

namespace ImGuiWrapper
{
    void setupImgui(Window &window);
    void beginDockspace();
    void ImGuiFrame(const std::function<void()> &func);
    void SetupStyle();

    template <typename Func>
    void Collapsable(const std::string &headerName, Func content);
};

template <typename Func>
void ImGuiWrapper::Collapsable(const std::string &headerName, Func content)
{
    if (ImGui::CollapsingHeader(headerName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent();
        content();
        ImGui::Unindent();
    }
}