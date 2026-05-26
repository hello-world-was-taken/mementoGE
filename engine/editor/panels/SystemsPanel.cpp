#include "editor/panels/SystemsPanel.h"

#include "core/ImGuiWrapper.h"
#include "core/Scene.h"

#include <imgui.h>

namespace
{
enum class SystemStatus
{
    Running,
    Disabled,
    Paused,
};

const char *statusLabel(SystemStatus status)
{
    switch (status)
    {
    case SystemStatus::Running:
        return "Running";
    case SystemStatus::Disabled:
        return "Disabled";
    case SystemStatus::Paused:
        return "Paused";
    }
    return "Unknown";
}

ImVec4 statusColor(SystemStatus status)
{
    switch (status)
    {
    case SystemStatus::Running:
        return ImVec4(0.38f, 0.76f, 0.46f, 1.0f);
    case SystemStatus::Disabled:
        return ImVec4(0.88f, 0.35f, 0.35f, 1.0f);
    case SystemStatus::Paused:
        return ImVec4(0.91f, 0.68f, 0.30f, 1.0f);
    }
    return ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

SystemStatus currentStatus(bool enabled, bool runtimePlaying, bool runtimeActive)
{
    if (!enabled)
        return SystemStatus::Disabled;
    if (runtimeActive && !runtimePlaying)
        return SystemStatus::Paused;
    return SystemStatus::Running;
}
} // namespace

SystemsPanel::SystemsPanel(EditorContext &ctx) : EditorPanel(ctx)
{
}

void SystemsPanel::draw()
{
    if (!m_ctx.workspaceState.showSystemsPanel)
        return;

    const bool wasOpen = m_ctx.workspaceState.showSystemsPanel;
    if (!ImGuiWrapper::BeginPanel("Systems", &m_ctx.workspaceState.showSystemsPanel))
    {
        ImGuiWrapper::EndPanel();
        if (wasOpen != m_ctx.workspaceState.showSystemsPanel)
            m_ctx.markEditorWorkspaceDirty();
        return;
    }

    Scene &scene = m_ctx.getActiveScene();
    const bool runtimePlaying = m_ctx.isRuntimePlaying();
    const bool runtimeActive = m_ctx.isRuntimeActive();

    ImGui::TextDisabled("Temporary overrides for the active scene");
    if (ImGui::Button("Enable all"))
    {
        scene.setAllSystemsEnabled(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Disable all"))
    {
        scene.setAllSystemsEnabled(false);
    }

    if (ImGui::BeginTable("SceneSystems", 3, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("System", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch, 1.4f);
        ImGui::TableSetupColumn("Last", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableHeadersRow();

        for (const auto &info : scene.getSystemInfo())
        {
            ImGui::PushID(info.name.c_str());
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            bool enabled = info.enabled;
            if (ImGui::Checkbox("##enabled", &enabled))
                scene.setSystemEnabled(info.name, enabled);
            ImGui::SameLine();
            ImGui::TextUnformatted(info.name.c_str());

            const SystemStatus status = currentStatus(info.enabled, runtimePlaying, runtimeActive);
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(statusColor(status), "%s", statusLabel(status));
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.3f ms", info.lastUpdateMilliseconds);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Rendering");
    ImGui::TextDisabled("Current mode: %s", runtimeActive ? "Play" : "Edit");

    uint32_t &rendererMask = runtimeActive ? m_ctx.runtimeRendererMask : m_ctx.editorRendererMask;
    auto drawRendererToggle = [&](const char *label, RendererType renderer)
    {
        const uint32_t flag = static_cast<uint32_t>(renderer);
        bool enabled = (rendererMask & flag) != 0;
        if (ImGui::Checkbox(label, &enabled))
        {
            if (enabled)
                rendererMask |= flag;
            else
                rendererMask &= ~flag;
            m_ctx.markEditorWorkspaceDirty();
        }
    };

    drawRendererToggle("Sprites, text, and particles", RendererType::Sprite);
    drawRendererToggle("Grid", RendererType::Grid);
    drawRendererToggle("Colliders and sensors", RendererType::Physics);
    drawRendererToggle("Selection and camera bounds", RendererType::Selection);

    if (ImGui::Button("Reset rendering for this mode"))
    {
        rendererMask = static_cast<uint32_t>(runtimeActive ? RendererType::Sprite : RendererType::All);
        m_ctx.markEditorWorkspaceDirty();
    }

    ImGuiWrapper::EndPanel();
    if (wasOpen != m_ctx.workspaceState.showSystemsPanel)
        m_ctx.markEditorWorkspaceDirty();
}
