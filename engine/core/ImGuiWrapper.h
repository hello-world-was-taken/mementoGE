#include "core/GLIncludes.h"

#include <functional>

namespace ImGuiWrapper
{
    void setupImgui(GLFWwindow *glfw_window);
    void beginDockspace();
    void ImGuiFrame(const std::function<void()> &func);
};