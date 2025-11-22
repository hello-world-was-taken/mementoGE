#include "editor/SceneListPanel.h"

#include "util/PathUtils.h"

#include <imgui.h>
#include <filesystem>

namespace fs = std::filesystem;

// Helper: gather YAML scene files
inline std::vector<std::string> getSceneFiles(const std::string &folderPath)
{
    std::vector<std::string> scenes;
    for (const auto &file : fs::directory_iterator(folderPath))
    {
        if (file.is_regular_file())
        {
            std::string ext = file.path().extension().string();
            if (ext == ".yaml" || ext == ".yml")
            {
                std::cout << "scene file name: " << file.path().string() << std::endl;
                scenes.push_back(file.path().string());
            }
        }
    }
    return scenes;
}

SceneListPanel::SceneListPanel(EditorContext &ctx) : EditorPanel{ctx}, m_ctx{ctx}
{
}

SceneListPanel::~SceneListPanel()
{
}

void SceneListPanel::draw()
{
    renderSceneListPanel();
}

void SceneListPanel::renderSceneListPanel()
{
    ImGui::Begin("Scenes");

    // find scenes/*.yaml
    auto sceneFiles = getSceneFiles(getGameScenesPath(""));

    for (const auto &scenePath : sceneFiles)
    {
        std::string fileName = fs::path(scenePath).filename().string();

        if (ImGui::Selectable(fileName.c_str(), m_ctx.selectedScenePath == scenePath))
        {
            m_ctx.selectedScenePath = scenePath;
        }
    }

    ImGui::End();
}
