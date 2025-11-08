#include "core/ImGuiWrapper.h"
#include "core/GLIncludes.h"
#include "core/IconsFontAwesome4.h"
#include "core/Window.h"

#include "util/PathUtils.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <filesystem>
#include <functional>
#include <imgui.h>

void ImGuiWrapper::setupImgui(Window &window)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
    // TODO: disabling now because of window positioning mismatch
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // TODO: for now
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    SetupStyle();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window.getGlfwWindow(), true);

    const char *glsl_version = "#version 150";
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void ImGuiWrapper::beginDockspace()
{
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        window_flags |= ImGuiWindowFlags_NoBackground;
    }

    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace Demo", nullptr, window_flags);

    if (!opt_padding)
        ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    ImGui::End();
}

void ImGuiWrapper::ImGuiFrame(const std::function<void()> &func)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    func();

    // End frame and render
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow *backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

void ImGuiWrapper::SetupStyle()
{
    ImGuiStyle &style = ImGui::GetStyle();

    // Flatness & rounding
    style.FrameRounding = 4.0f; // roundness for frames (buttons, sliders ...)
    style.WindowRounding = 6.0f;
    style.PopupRounding = 4.0f;
    style.GrabRounding = 4.0f; // Slider grabber
    style.ScrollbarRounding = 12.0f;

    // Spacing & padding
    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(6.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);

    // To minimize borders for a flatter look
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;

    // Color palette (normalized color values (0.0 to 1.0))
    ImVec4 accent_color = ImVec4(0.314f, 0.447f, 0.655f, 1.00f); // Blue-ish accent
    ImVec4 lighter_accent_color = ImVec4(accent_color.x + 0.1f, accent_color.y + 0.1f, accent_color.z + 0.1f, 1.0f);
    ImVec4 bg_dark = ImVec4(0.051f, 0.039f, 0.043f, 1.00f);
    ImVec4 bg_medium = ImVec4(0.12f, 0.11f, 0.13f, 1.00f); // Soft step above bg_dark
    ImVec4 bg_light = ImVec4(0.22f, 0.20f, 0.24f, 1.00f);  // Lightest neutral background
    ImVec4 text_main = ImVec4(0.85f, 0.85f, 0.88f, 1.00f); // Slightly cool to fit blue accent

    // Windows
    style.Colors[ImGuiCol_WindowBg] = bg_dark;
    style.Colors[ImGuiCol_TitleBgActive] = bg_medium;
    style.Colors[ImGuiCol_TitleBg] = bg_dark;
    style.Colors[ImGuiCol_Text] = text_main;

    // Frames (Buttons, Checkboxes, Sliders)
    style.Colors[ImGuiCol_FrameBg] = bg_medium;
    style.Colors[ImGuiCol_FrameBgHovered] = bg_light;
    style.Colors[ImGuiCol_FrameBgActive] = bg_light;

    // Buttons (using the accent color)
    style.Colors[ImGuiCol_Button] = accent_color;
    style.Colors[ImGuiCol_ButtonHovered] = lighter_accent_color;
    style.Colors[ImGuiCol_ButtonActive] = accent_color;

    // Tab-Specific Colors:
    style.Colors[ImGuiCol_Tab] = bg_medium;
    style.Colors[ImGuiCol_TabHovered] = lighter_accent_color;
    style.Colors[ImGuiCol_TabActive] = accent_color;
    style.Colors[ImGuiCol_TabUnfocused] = bg_medium;
    style.Colors[ImGuiCol_TabUnfocusedActive] = accent_color;

    // Scrollbar/Check Mark/Grab
    style.Colors[ImGuiCol_ScrollbarBg] = bg_dark;
    style.Colors[ImGuiCol_ScrollbarGrab] = bg_light;
    style.Colors[ImGuiCol_CheckMark] = accent_color;
    style.Colors[ImGuiCol_SliderGrab] = accent_color;
    style.Colors[ImGuiCol_SliderGrabActive] = accent_color;

    // Styling selectable (it uses headers styles)
    style.Colors[ImGuiCol_Header] = bg_medium;
    style.Colors[ImGuiCol_HeaderHovered] = lighter_accent_color;
    style.Colors[ImGuiCol_HeaderActive] = accent_color;

    // Setup fonts
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontDefault();

    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced
    static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    std::filesystem::path fontPath = getFilePath("fonts/fontawesome-webfont.ttf");
    io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 13.0f, &config, icon_ranges);
}