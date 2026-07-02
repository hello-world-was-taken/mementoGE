#pragma once

#include "core/components/Sprite.h"

#include "core/InputSnapshot.h"

#include "core/GLIncludes.h"
#include "core/Window.h"

#include <functional>

namespace ImGuiWrapper
{
void setupImgui(Window &window);
void shutdownImgui();
void beginDockspace();
void buildInitialLayout();
void applySmoothScrolling(const InputSnapshot &input);
void ImGuiFrame(const InputSnapshot &input, const std::function<void()> &func);
void SetupStyle();
void SetupFonts();

// resuable components
struct Padding
{
    float left = 0.0f;
    float right = 0.0f;
    float top = 0.0f;
    float bottom = 0.0f;
};

struct Margin
{
    float left = 0.0f;
    float right = 0.0f;
    float top = 0.0f;
    float bottom = 0.0f;
};

bool BeginPanel(const char *name, bool *open = nullptr, ImGuiWindowFlags flags = 0);
void EndPanel();
bool Button(const char *label, const ImVec2 &size = ImVec2(0.0f, 0.0f));
bool ToolbarSelectable(
    const char *label, bool selected, const char *tooltip = nullptr, const ImVec2 &size = ImVec2(13.0f, 0.0f));
bool TreeNodeEx(const void *id, const char *label, ImGuiTreeNodeFlags flags = 0,
                const Padding &padding = Padding());
void TreePop();
bool Row(const char *text, const char *icon = nullptr, const char *buttonLabel = nullptr,
         bool buttonEnabled = true, const Padding &padding = Padding(), const Margin &margin = Margin());
bool InputTextSimple(const char *label, std::string &value);
bool ImageButtonFixedHeight(Sprite &sprite);
std::pair<bool, glm::vec3> PixelAwareImageButton(Sprite &sprite, float fixed_height = 32.0f);

void Collapsable(const std::string &headerName, const std::function<void()> &content);
}; // namespace ImGuiWrapper
