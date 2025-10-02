#include "core/GLIncludes.h"
#include "core/Window.h"

#include <functional>

namespace ImGuiWrapper
{
    void setupImgui(Window &window);
    void beginDockspace();
    void ImGuiFrame(const std::function<void()> &func);
};