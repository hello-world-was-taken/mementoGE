#include "core/components/Sprite.h"

#include "core/GLIncludes.h"
#include "core/IconsFontAwesome4.h"
#include "core/ImGuiWrapper.h"
#include "core/Window.h"

#include "util/LogChannels.h"
#include "util/PathUtils.h"

#include <algorithm>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <filesystem>
#include <functional>
#include <imgui.h>
#include <imgui/imgui_internal.h>
#include <memory>
#include <sawyer/Logger.h>
#include <string>

namespace
{
// ImGuiIO::IniFilename stores a raw const char*, so the backing string must outlive setupImgui().
std::string g_imguiIniPath;

const ImVec4 kColorBgDark{0.086f, 0.094f, 0.114f, 1.0f};
const ImVec4 kColorBgPanel{0.118f, 0.129f, 0.157f, 1.0f};
const ImVec4 kColorBgRaised{0.161f, 0.176f, 0.212f, 1.0f};
const ImVec4 kColorBgHover{0.204f, 0.227f, 0.275f, 1.0f};
const ImVec4 kColorBorder{0.212f, 0.235f, 0.282f, 1.0f};
const ImVec4 kColorAccent{0.298f, 0.553f, 1.0f, 1.0f};
const ImVec4 kColorAccentHover{0.416f, 0.631f, 1.0f, 1.0f};
const ImVec4 kColorAccentSelected{0.145f, 0.298f, 0.478f, 1.0f};
const ImVec4 kColorText{0.902f, 0.918f, 0.941f, 1.0f};
const ImVec4 kColorTextDisabled{0.541f, 0.576f, 0.639f, 1.0f};
const ImVec4 kColorDangerText{0.878f, 0.388f, 0.424f, 1.0f};
const ImVec4 kColorDangerHover{0.322f, 0.122f, 0.145f, 1.0f};
const ImVec4 kColorDangerActive{0.431f, 0.145f, 0.169f, 1.0f};
} // namespace

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
    // FIXME: disabling keyboard nav to avoid menubar
    // element selection in play mode
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // TODO: for now
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    std::filesystem::create_directories(getMementoEditorStateDir());
    g_imguiIniPath = (getMementoEditorStateDir() / "imgui.ini").string();
    io.IniFilename = g_imguiIniPath.c_str();

    SetupStyle();
    SetupFonts();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window.getGlfwWindow(), true);

    const char *glsl_version = "#version 150";
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void ImGuiWrapper::shutdownImgui()
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.IniFilename != nullptr)
    {
        ImGui::SaveIniSettingsToDisk(io.IniFilename);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    }

    ImGui::Begin("DockSpace Demo", nullptr, window_flags);

    if (!opt_padding)
    {
        ImGui::PopStyleVar();
    }

    if (opt_fullscreen)
    {
        ImGui::PopStyleVar(2);
    }

    ImGuiIO &io = ImGui::GetIO();
    if (!(io.ConfigFlags & ImGuiConfigFlags_DockingEnable))
    {
        return;
    }

    ImGuiID dockspace_id = ImGui::GetID("MementoDockspace");
    buildInitialLayout();
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    ImGui::End();
}

void ImGuiWrapper::buildInitialLayout()
{
    ImGuiIO &io = ImGui::GetIO();
    if (!(io.ConfigFlags & ImGuiConfigFlags_DockingEnable))
    {
        return;
    }

    // If an ini file already exists, let ImGui load and manage layout
    // rather than overriding it with a hard-coded default.
    const char *iniFilename = io.IniFilename;
    if (iniFilename != nullptr && std::filesystem::exists(iniFilename))
    {
        return;
    }

    static bool initialized = false;
    if (initialized)
    {
        return;
    }
    initialized = true;

    ImGuiID dockspace_id = ImGui::GetID("MementoDockspace");
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    LOG_DEBUG(LogChannel::ImGui,
        "Main viewport size: {}x{}",
        ImGui::GetMainViewport()->Size.x,
        ImGui::GetMainViewport()->Size.y);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

    ImGuiID dock_main_id = dockspace_id;
    ImGuiID dock_right_id;
    ImGuiID dock_left_id;
    ImGuiID dock_bottom_id;
    ImGuiID dock_top_id;
    ImGuiID dock_top_left_id;
    ImGuiID dock_top_center_id;
    ImGuiID dock_right_top_id;
    ImGuiID dock_right_bottom_id;

    ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.24f, &dock_right_id, &dock_main_id);
    dock_left_id = dock_main_id;

    ImGui::DockBuilderSplitNode(dock_left_id, ImGuiDir_Down, 0.27f, &dock_bottom_id, &dock_left_id);
    dock_top_id = dock_left_id;

    ImGui::DockBuilderSplitNode(dock_top_id, ImGuiDir_Left, 0.19f, &dock_top_left_id, &dock_top_center_id);

    ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down, 0.37f, &dock_right_bottom_id, &dock_right_top_id);

    ImGui::DockBuilderDockWindow("Hierarchy", dock_top_left_id);
    ImGui::DockBuilderDockWindow("###ScenePanel", dock_top_center_id);

    ImGui::DockBuilderDockWindow("Assets", dock_bottom_id);
    ImGui::DockBuilderDockWindow("Log", dock_bottom_id);
    ImGui::DockBuilderDockWindow("Performance Stats", dock_bottom_id);
    ImGui::DockBuilderDockWindow("Dear ImGui Metrics/Debugger", dock_bottom_id);
    ImGui::DockBuilderDockWindow("Sprite Sheet Editor", dock_bottom_id);

    ImGui::DockBuilderDockWindow("Properties", dock_right_top_id);
    ImGui::DockBuilderDockWindow("Context", dock_right_top_id);
    ImGui::DockBuilderDockWindow("Bound Textures", dock_right_bottom_id);

    ImGui::DockBuilderFinish(dockspace_id);
}

void ImGuiWrapper::applySmoothScrolling(const InputSnapshot &input)
{
    ImGuiIO &io = ImGui::GetIO();

    const float scrollMultiplier = 0.5f;
    const float scrollSmoothing = 8.0f;
    static ImVec2 scrollEnergy = ImVec2(0.0f, 0.0f);

    glm::vec2 scrollDelta{0.0f};
    for (const Event &event : input.events)
    {
        if (event.type == EventType::MouseScroll)
        {
            scrollDelta += glm::vec2{event.scrollX, event.scrollY};
        }
    }

    scrollDelta.x *= scrollMultiplier;
    scrollDelta.y *= scrollMultiplier;
    // Immediately stop if direction changes
    if (scrollEnergy.x * scrollDelta.x < 0.0f)
    {
        scrollEnergy.x = 0.0f;
    }
    if (scrollEnergy.y * scrollDelta.y < 0.0f)
    {
        scrollEnergy.y = 0.0f;
    }
    scrollEnergy.x += scrollDelta.x;
    scrollEnergy.y += scrollDelta.y;

    // Apply smooth scrolling (MUST be before ImGui::NewFrame())
    ImVec2 scroll_now = ImVec2(0.0f, 0.0f);
    if (std::abs(scrollEnergy.x) > 0.01f)
    {
        scroll_now.x = scrollEnergy.x * io.DeltaTime * scrollSmoothing;
        scrollEnergy.x -= scroll_now.x;
    }
    else
    {
        // Cutoff smoothing when it's basically stopped
        scrollEnergy.x = 0.0f;
    }
    if (std::abs(scrollEnergy.y) > 0.01f)
    {
        scroll_now.y = scrollEnergy.y * io.DeltaTime * scrollSmoothing;
        scrollEnergy.y -= scroll_now.y;
    }
    else
    {
        // Cutoff smoothing when it's basically stopped
        scrollEnergy.y = 0.0f;
    }

    io.AddMouseWheelEvent(-scroll_now.x, scroll_now.y);
}

void ImGuiWrapper::ImGuiFrame(const InputSnapshot &input, const std::function<void()> &func)
{
    applySmoothScrolling(input);

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

    // Compact, rectangular editor geometry.
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 1.0f;
    style.PopupRounding = 2.0f;
    style.GrabRounding = 1.0f;
    style.ScrollbarRounding = 1.0f;
    style.TabRounding = 1.0f;

    // Spacing & padding
    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(6.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);

    // Thin borders keep adjacent dark surfaces legible.
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.TabBorderSize = 0.0f;

    // Windows and text
    style.Colors[ImGuiCol_Text] = kColorText;
    style.Colors[ImGuiCol_TextDisabled] = kColorTextDisabled;
    style.Colors[ImGuiCol_WindowBg] = kColorBgDark;
    style.Colors[ImGuiCol_ChildBg] = kColorBgDark;
    style.Colors[ImGuiCol_PopupBg] = kColorBgPanel;
    style.Colors[ImGuiCol_TitleBg] = kColorBgDark;
    style.Colors[ImGuiCol_TitleBgActive] = kColorBgPanel;
    style.Colors[ImGuiCol_TitleBgCollapsed] = kColorBgDark;
    style.Colors[ImGuiCol_MenuBarBg] = kColorBgPanel;
    style.Colors[ImGuiCol_Border] = kColorBorder;

    // Frames (Buttons, Checkboxes, Sliders)
    style.Colors[ImGuiCol_FrameBg] = kColorBgPanel;
    style.Colors[ImGuiCol_FrameBgHovered] = kColorBgRaised;
    style.Colors[ImGuiCol_FrameBgActive] = kColorBgHover;

    // Keep ordinary controls neutral; reserve blue for selection and focus.
    style.Colors[ImGuiCol_Button] = kColorBgRaised;
    style.Colors[ImGuiCol_ButtonHovered] = kColorBgHover;
    style.Colors[ImGuiCol_ButtonActive] = kColorAccentSelected;

    // Tabs
    style.Colors[ImGuiCol_Tab] = kColorBgPanel;
    style.Colors[ImGuiCol_TabHovered] = kColorBgHover;
    style.Colors[ImGuiCol_TabActive] = kColorAccentSelected;
    style.Colors[ImGuiCol_TabUnfocused] = kColorBgDark;
    style.Colors[ImGuiCol_TabUnfocusedActive] = kColorBgRaised;

    // Scrollbar/Check Mark/Grab
    style.Colors[ImGuiCol_ScrollbarBg] = kColorBgDark;
    style.Colors[ImGuiCol_ScrollbarGrab] = kColorBgRaised;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = kColorBgHover;
    style.Colors[ImGuiCol_ScrollbarGrabActive] = kColorAccentSelected;
    style.Colors[ImGuiCol_CheckMark] = kColorAccent;
    style.Colors[ImGuiCol_SliderGrab] = kColorAccent;
    style.Colors[ImGuiCol_SliderGrabActive] = kColorAccentHover;

    // Selectables, separators and docking affordances
    style.Colors[ImGuiCol_Header] = kColorBgRaised;
    style.Colors[ImGuiCol_HeaderHovered] = kColorBgHover;
    style.Colors[ImGuiCol_HeaderActive] = kColorAccentSelected;
    style.Colors[ImGuiCol_Separator] = kColorBorder;
    style.Colors[ImGuiCol_SeparatorHovered] = kColorAccent;
    style.Colors[ImGuiCol_SeparatorActive] = kColorAccentHover;
    style.Colors[ImGuiCol_DockingPreview] = kColorAccent;
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(kColorAccent.x, kColorAccent.y, kColorAccent.z, 0.35f);
    style.Colors[ImGuiCol_NavCursor] = kColorAccent;
}

void ImGuiWrapper::SetupFonts()
{
    ImGuiIO &io = ImGui::GetIO();

    const std::string monoPath = getGameAssetPath("fonts/JetBrains_Mono/static/JetBrainsMono-Regular.ttf").string();
    ImFont *uiFont = io.Fonts->AddFontFromFileTTF(monoPath.c_str(), 15.0f);

    if (uiFont == nullptr)
    {
        uiFont = io.Fonts->AddFontDefault();
    }

    io.FontDefault = uiFont;

    ImFontConfig iconConfig;
    iconConfig.MergeMode = true;
    iconConfig.DstFont = uiFont;
    iconConfig.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced

    static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

    std::filesystem::path iconPath = getEngineAssetPath("fonts/fontawesome-webfont.ttf");

    io.Fonts->AddFontFromFileTTF(iconPath.c_str(), 13.0f, &iconConfig, icon_ranges);
    io.Fonts->Flags |= ImFontFlags_ImplicitRefSize;
}

bool ImGuiWrapper::BeginPanel(const char *name, bool *open, ImGuiWindowFlags flags)
{
    return ImGui::Begin(name, open, flags);
}

void ImGuiWrapper::EndPanel()
{
    ImGui::End();
}

bool ImGuiWrapper::Button(const char *label, const ImVec2 &size)
{
    return ImGui::Button(label, size);
}

bool ImGuiWrapper::ToolbarSelectable(const char *label, bool selected, const char *tooltip, const ImVec2 &size)
{
    ImGui::PushStyleColor(ImGuiCol_Header, kColorAccentSelected);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, selected ? kColorAccent : kColorBgHover);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, kColorAccentHover);
    ImGui::PushStyleColor(ImGuiCol_Text, selected ? kColorText : kColorTextDisabled);

    const bool pressed = ImGui::Selectable(label, selected, 0, size);

    ImGui::PopStyleColor(4);

    if (selected)
    {
        const ImVec2 itemMin = ImGui::GetItemRectMin();
        const ImVec2 itemMax = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddLine(ImVec2(itemMin.x, itemMax.y - 1.0f),
            ImVec2(itemMax.x, itemMax.y - 1.0f),
            ImGui::GetColorU32(kColorAccent),
            2.0f);
    }

    if (ImGui::IsItemHovered() && tooltip != nullptr && tooltip[0] != '\0')
    {
        ImGui::SetTooltip("%s", tooltip);
    }

    return pressed;
}

bool ImGuiWrapper::TreeNodeEx(
    const void *id, const char *label, ImGuiTreeNodeFlags flags, const ImGuiWrapper::Padding &padding)
{
    ImGuiStyle &style = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
        ImVec2(padding.left > padding.right ? padding.left : padding.right,
            padding.top > padding.bottom ? padding.top : padding.bottom));

    const bool opened = ImGui::TreeNodeEx(id, flags | ImGuiTreeNodeFlags_FramePadding, "%s", label);

    ImGui::PopStyleVar(2);

    return opened;
}

void ImGuiWrapper::TreePop()
{
    ImGui::TreePop();
}

bool ImGuiWrapper::Row(const char *text,
    const char *icon,
    const char *buttonLabel,
    bool buttonEnabled,
    const ImGuiWrapper::Padding &padding,
    const ImGuiWrapper::Margin &margin)
{
    ImGuiStyle &style = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, 0.0f));

    const bool hasIcon = icon != nullptr && icon[0] != '\0';
    const bool hasButton = buttonLabel != nullptr && buttonLabel[0] != '\0';
    constexpr float rowGap = 2.0f;
    const float outerAvailableWidth = ImGui::GetContentRegionAvail().x;
    const ImVec2 outerMin = ImGui::GetCursorScreenPos();
    const float availableWidth = outerAvailableWidth - margin.left - margin.right;
    const float rowWidth = availableWidth > 0.0f ? availableWidth : 0.0f;
    const ImVec2 rowMin = ImVec2(outerMin.x + margin.left, outerMin.y + margin.top);
    const ImVec2 buttonTextSize = hasButton ? ImGui::CalcTextSize(buttonLabel) : ImVec2(0.0f, 0.0f);
    const std::string displayText = hasIcon ? std::string(icon) + "  " + text : std::string(text);
    const ImVec2 contentStartScreenPos = ImVec2(rowMin.x + padding.left, rowMin.y + padding.top);
    const float rowTextHeight = ImGui::GetTextLineHeight() + (style.FramePadding.y * 2.0f);
    const float rowButtonHeight = hasButton ? buttonTextSize.y + (style.FramePadding.y * 2.0f) : 0.0f;
    const float rowHeight = rowTextHeight > rowButtonHeight ? rowTextHeight : rowButtonHeight;
    const ImVec2 rowMax = ImVec2(rowMin.x + rowWidth, rowMin.y + padding.top + rowHeight + padding.bottom);
    float buttonWidth = hasButton ? buttonTextSize.x + (style.FramePadding.x * 2.0f) : 0.0f;
    buttonWidth = buttonWidth > rowButtonHeight ? buttonWidth : rowButtonHeight;
    const float buttonX = rowMax.x - padding.right - buttonWidth;
    const float labelRegionWidth = hasButton ? std::max(0.0f, buttonX - style.ItemSpacing.x - rowMin.x) : rowWidth;

    ImGui::PushID(text);
    ImGui::SetCursorScreenPos(rowMin);
    const bool rowSelected =
        ImGui::Selectable("##row_selectable", false, 0, ImVec2(labelRegionWidth, rowMax.y - rowMin.y));

    ImGui::SetCursorScreenPos(contentStartScreenPos);
    ImGui::AlignTextToFramePadding();
    ImGui::PushClipRect(rowMin, ImVec2(rowMin.x + labelRegionWidth, rowMax.y), true);
    ImGui::TextUnformatted(displayText.c_str());
    ImGui::PopClipRect();

    if (!hasButton)
    {
        ImGui::SetCursorScreenPos(ImVec2(outerMin.x, rowMax.y + rowGap + margin.bottom));
        ImGui::Dummy(ImVec2(0.0f, 0.0f));
        ImGui::PopID();
        ImGui::PopStyleVar();
        return rowSelected;
    }

    const float buttonHeight = buttonTextSize.y + (style.FramePadding.y * 2.0f);
    const float buttonY = rowMin.y + padding.top + ((rowHeight - buttonHeight) * 0.5f);
    ImGui::SetCursorScreenPos(ImVec2(buttonX, buttonY));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    int pushedButtonColors = 1;
    if (buttonEnabled)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, kColorDangerText);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kColorDangerHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, kColorDangerActive);
        pushedButtonColors += 3;
    }
    if (!buttonEnabled)
    {
        ImGui::BeginDisabled();
    }
    const bool pressed = ImGui::Button(buttonLabel, ImVec2(buttonWidth, buttonHeight));
    if (!buttonEnabled)
    {
        ImGui::EndDisabled();
    }
    ImGui::PopStyleColor(pushedButtonColors);

    ImGui::SetCursorScreenPos(ImVec2(outerMin.x, rowMax.y + rowGap + margin.bottom));
    ImGui::Dummy(ImVec2(0.0f, 0.0f));
    ImGui::PopID();
    ImGui::PopStyleVar();

    return buttonEnabled && pressed;
}

void ImGuiWrapper::Collapsable(const std::string &headerName, const std::function<void()> &content)
{
    if (ImGui::CollapsingHeader(headerName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent();
        content();
        ImGui::Unindent();
    }
}

bool ImGuiWrapper::InputTextSimple(const char *label, std::string &value)
{
    char buf[256];

    memset(buf, 0, sizeof(buf));                  // clear buffer
    strncpy(buf, value.c_str(), sizeof(buf) - 1); // copy current text into buffer

    if (ImGui::InputText(label, buf, sizeof(buf)))
    {
        value = buf;
        return true;
    }

    return false;
}

// Default height is 32.0f
bool ImGuiWrapper::ImageButtonFixedHeight(Sprite &sprite)
{
    float fixed_height = 64.0f;
    const ImVec4 tintColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    const ImVec4 bgColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    std::shared_ptr<Texture> &texture = sprite.texture;
    float texW = texture->getWidth();
    float texH = texture->getHeight();
    unsigned int texId = texture->getTextureId();

    std::array<glm::vec2, 4> textureCoordinates = sprite.getNormalizedTextureCoordinates();
    glm::vec2 uv0 = textureCoordinates[0]; // topLeft
    glm::vec2 uv1 = textureCoordinates[2]; // bottomRight

    ImGui::PushID(sprite.getId());
    // Protect against invalid texture size
    if (texW <= 0.0f || texH <= 0.0f)
    {
        // fallback: draw a small square placeholder button
        bool clicked = ImGui::ImageButton("invalidsize",
            texId,
            ImVec2(fixed_height, fixed_height),
            ImVec2(uv0[0], uv0[1]),
            ImVec2(uv1[0], uv1[1]),
            ImVec4(0, 0, 0, 0),
            tintColor);

        ImGui::PopID();
        return clicked;
    }

    const float aspect = texW / texH;
    const float width = fixed_height * aspect;
    bool isClicked = ImGui::ImageButton(
        "", texId, ImVec2(width, fixed_height), ImVec2(uv0[0], uv0[1]), ImVec2(uv1[0], uv1[1]), bgColor, tintColor);

    ImGui::PopID();
    return isClicked;
}

std::pair<bool, glm::vec3> ImGuiWrapper::PixelAwareImageButton(Sprite &sprite, float fixed_height)
{
    const ImVec4 tintColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    const ImVec4 bgColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    std::shared_ptr<Texture> &texture = sprite.texture;
    float texW = texture->getWidth();
    float texH = texture->getHeight();
    unsigned int texId = texture->getTextureId();

    std::array<glm::vec2, 4> textureCoordinates = sprite.getNormalizedTextureCoordinates();
    glm::vec2 uv0 = textureCoordinates[0]; // topLeft
    glm::vec2 uv1 = textureCoordinates[2]; // bottomRight

    glm::vec3 colorAtPixel(0, 0, 0); // default to black if not hovered or invalid size

    ImGui::PushID(sprite.getId());
    // Protect against invalid texture size
    if (texW <= 0.0f || texH <= 0.0f)
    {
        // fallback: draw a small square placeholder button
        bool clicked = ImGui::ImageButton("invalidsize",
            texId,
            ImVec2(fixed_height, fixed_height),
            ImVec2(uv0[0], uv0[1]),
            ImVec2(uv1[0], uv1[1]),
            ImVec4(0, 0, 0, 0),
            tintColor);

        ImGui::PopID();
        return {clicked, colorAtPixel};
    }

    const float aspect = texW / texH;
    const float width = fixed_height * aspect;

    // Remove the default style padding and color to make it easier to get accurate pixel coordinates and not have to
    // worry about padding
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

    bool isClicked = ImGui::ImageButton(
        "", texId, ImVec2(width, fixed_height), ImVec2(uv0[0], uv0[1]), ImVec2(uv1[0], uv1[1]), bgColor, tintColor);

    // remove the style changes after the button so they don't affect other UI elements
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);

    // ImGui::Image(texId, {width, fixed_height}, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
    if (ImGui::IsItemHovered())
    {
        // Get mouse position relative to the top-left corner of the image button
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 buttonPos = ImGui::GetItemRectMin();

        ImVec2 relativePos = ImVec2(mousePos.x - buttonPos.x, mousePos.y - buttonPos.y);

        // Convert relative position to pixel coordinates in the texture
        int pixelX = static_cast<int>((relativePos.x / width) * texW);
        int pixelY = static_cast<int>((relativePos.y / fixed_height) * texH);

        colorAtPixel = texture->getColorAtPixel(pixelX, pixelY);
    }

    ImGui::PopID();
    return {false, colorAtPixel};
}
