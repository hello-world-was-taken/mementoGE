#include "editor/panels/LogPanel.h"

#include "core/ImGuiWrapper.h"

#include <chrono>
#include <ctime>
#include <imgui.h>
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

ImVec4 levelColor(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Debug:
        return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
    case LogLevel::Info:
        return ImGui::GetStyleColorVec4(ImGuiCol_Text);
    case LogLevel::Warn:
        return ImVec4(0.91f, 0.68f, 0.30f, 1.0f);
    case LogLevel::Error:
        return ImVec4(0.95f, 0.35f, 0.35f, 1.0f);
    }
    return ImGui::GetStyleColorVec4(ImGuiCol_Text);
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

    if (ImGui::Button("Clear"))
    {
        Logger::clearRingBuffer();
    }
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

    const std::vector<LogEntry> entries = Logger::getRingBufferSnapshot();
    std::vector<const LogEntry *> visibleEntries;
    visibleEntries.reserve(entries.size());

    for (const LogEntry &entry : entries)
    {
        const bool levelVisible = (entry.level == LogLevel::Debug && m_showDebug) ||
                                  (entry.level == LogLevel::Info && m_showInfo) ||
                                  (entry.level == LogLevel::Warn && m_showWarnings) ||
                                  (entry.level == LogLevel::Error && m_showErrors);
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

        visibleEntries.push_back(&entry);
    }

    if (ImGui::BeginChild("LogEntries", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar))
    {
        const bool shouldScrollToBottom =
            m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(visibleEntries.size()));
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
                const LogEntry &entry = *visibleEntries[static_cast<size_t>(i)];
                char timestamp[16]{};
                formatTimestamp(entry, timestamp);

                ImGui::PushID(i);
                ImGui::TextColored(levelColor(entry.level),
                    "[%s] [%-5s] [%s] %s",
                    timestamp,
                    levelName(entry.level),
                    entry.channel.c_str(),
                    entry.message.c_str());
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s:%d", entry.filePath.c_str(), entry.line);
                }
                ImGui::PopID();
            }
        }

        if (shouldScrollToBottom)
        {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();

    ImGuiWrapper::EndPanel();
    if (wasOpen != m_ctx.workspaceState.showLogPanel)
    {
        m_ctx.markEditorWorkspaceDirty();
    }
}
