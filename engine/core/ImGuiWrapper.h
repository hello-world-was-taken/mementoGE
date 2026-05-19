#pragma once

#include "core/components/Sprite.h"

#include "core/GLIncludes.h"
#include "core/Window.h"

#include <functional>

namespace ImGuiWrapper
{
void setupImgui(Window &window);
void beginDockspace();
void buildInitialLayout();
void ImGuiFrame(const std::function<void()> &func);
void SetupStyle();

// resuable components
bool InputTextSimple(const char *label, std::string &value);
bool ImageButtonFixedHeight(Sprite &sprite);

template <typename Func> void Collapsable(const std::string &headerName, Func content);
}; // namespace ImGuiWrapper

template <typename Func> void ImGuiWrapper::Collapsable(const std::string &headerName, Func content)
{
    if (ImGui::CollapsingHeader(headerName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent();
        content();
        ImGui::Unindent();
    }
}