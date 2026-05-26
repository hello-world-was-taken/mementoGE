#include "editor/panels/LogPanel.h"

#include "core/ImGuiWrapper.h"

#include <chrono>
#include <ctime>
#include <imgui.h>
#include <imgui_internal.h>
#include <sawyer/Logger.h>
#include <string>
#include <vector>

namespace
{
const char *levelName(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warn:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    }
    return "UNKNOWN";
}

void formatTimestamp(const LogEntry &entry, char (&buffer)[16])
{
    const std::time_t timestamp = std::chrono::system_clock::to_time_t(entry.timestamp);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &timestamp);
#else
    localtime_r(&timestamp, &localTime);
#endif
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &localTime);
}

std::string formatEntry(const LogEntry &entry)
{
    char timestamp[16]{};
    formatTimestamp(entry, timestamp);

    std::string level = levelName(entry.level);
    level.resize(5, ' ');

    return "[" + std::string(timestamp) + "] [" + level + "] [" + entry.channel + "] " + entry.message;
}

} // namespace

LogPanel::LogPanel(EditorContext &ctx) : EditorPanel(ctx)
{
}

void LogPanel::draw()
{
    if (!m_ctx.workspaceState.showLogPanel)
    {
        return;
    }

    const bool wasOpen = m_ctx.workspaceState.showLogPanel;
    if (!ImGuiWrapper::BeginPanel("Log", &m_ctx.workspaceState.showLogPanel))
    {
        ImGuiWrapper::EndPanel();
        if (wasOpen != m_ctx.workspaceState.showLogPanel)
        {
            m_ctx.markEditorWorkspaceDirty();
        }
        return;
    }

    std::vector<LogEntry> entries = Logger::getRingBufferSnapshot();

    if (ImGui::Button("Clear"))
    {
        Logger::clearRingBuffer();
        entries.clear();
    }
    ImGui::SameLine();
    const bool copyAllRequested = ImGui::Button("Copy all");
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);
    ImGui::SameLine();
    ImGui::Checkbox("Debug", &m_showDebug);
    ImGui::SameLine();
    ImGui::Checkbox("Info", &m_showInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Warnings", &m_showWarnings);
    ImGui::SameLine();
    ImGui::Checkbox("Errors", &m_showErrors);

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##LogFilter", "Filter by message, channel, or file...", m_filter.data(), m_filter.size());
    ImGui::Separator();

    std::string nextLogText;

    for (const LogEntry &entry : entries)
    {
        const bool levelVisible =
            (entry.level == LogLevel::Debug && m_showDebug) || (entry.level == LogLevel::Info && m_showInfo) ||
            (entry.level == LogLevel::Warn && m_showWarnings) || (entry.level == LogLevel::Error && m_showErrors);
        if (!levelVisible)
        {
            continue;
        }

        if (m_filter[0] != '\0' && entry.message.find(m_filter.data()) == std::string::npos &&
            entry.channel.find(m_filter.data()) == std::string::npos &&
            entry.filePath.find(m_filter.data()) == std::string::npos)
        {
            continue;
        }

        if (!nextLogText.empty())
        {
            nextLogText += '\n';
        }
        nextLogText += formatEntry(entry);
    }

    const bool textChanged = nextLogText != m_logText;
    m_logText = std::move(nextLogText);

    if (copyAllRequested && !m_logText.empty())
    {
        ImGui::SetClipboardText(m_logText.c_str());
    }

    const ImGuiID logTextId = ImGui::GetID("##LogText");
    ImGuiWindow *logTextWindow = ImGui::FindWindowByID(logTextId);
    const bool wasAtBottom = logTextWindow == nullptr || logTextWindow->Scroll.y >= logTextWindow->ScrollMax.y - 1.0f;

    ImGui::InputTextMultiline("##LogText",
        const_cast<char *>(m_logText.c_str()),
        m_logText.size() + 1,
        ImGui::GetContentRegionAvail(),
        ImGuiInputTextFlags_ReadOnly);

    if (m_autoScroll && textChanged && wasAtBottom)
    {
        logTextWindow = ImGui::FindWindowByID(logTextId);
        if (logTextWindow != nullptr)
        {
            ImGui::SetScrollY(logTextWindow, logTextWindow->ScrollMax.y);
        }
    }

    ImGuiWrapper::EndPanel();
    if (wasOpen != m_ctx.workspaceState.showLogPanel)
    {
        m_ctx.markEditorWorkspaceDirty();
    }
}
