#include "editor/AssetsPanel.h"

#include "core/IconsFontAwesome4.h"

#include "util/PathUtils.h"

#include <filesystem>
#include <imgui.h>

namespace fs = std::filesystem;

struct AssetItem
{
    std::string name;
    bool isDirectory;
    std::string extension;
};

inline std::vector<AssetItem> getDirectoryContents(const std::string &path)
{
    std::vector<AssetItem> items;

    for (const auto &entry : fs::directory_iterator(path))
    {
        AssetItem item;
        item.name = entry.path().filename().string();
        item.isDirectory = entry.is_directory();
        item.extension = entry.path().extension().string();
        items.push_back(item);
    }

    return items;
}

inline const char *getIconForItem(const AssetItem &item)
{
    if (item.isDirectory)
        return ICON_FA_FOLDER;

    // file extension icons
    if (item.extension == ".png" || item.extension == ".jpg")
        return ICON_FA_FILE_IMAGE_O;

    if (item.extension == ".json" || item.extension == ".txt")
        return ICON_FA_FILE_TEXT_O;

    if (item.extension == ".wav" || item.extension == ".ogg")
        return ICON_FA_FILE_AUDIO_O;

    // fallback
    return ICON_FA_FILE_O;
}

//TODO: DO WE NEED THIS?
inline std::vector<std::string> getFoldersInDirectory(const std::string &path)
{
    std::vector<std::string> folders;

    for (const auto &entry : std::filesystem::directory_iterator(path))
    {
        if (entry.is_directory())
        {
            folders.push_back(entry.path().filename().string());
        }
    }

    return folders;
}

AssetsPanel::AssetsPanel(EditorContext &ctx) : EditorPanel{ctx}, m_ctx{ctx}
{
}

AssetsPanel::~AssetsPanel()
{
}

void AssetsPanel::draw()
{
    renderAssetsPanel();
}

void AssetsPanel::renderAssetsPanel()
{
    ImGui::Begin("Assets");

    ImGui::Columns(2, nullptr, true); // two-column layout

    // left columns asset folders
    auto folders = getFoldersInDirectory(GAME_ASSETS_DIR);

    for (const auto &folder : folders)
    {
        bool selected = (m_ctx.selectedAssetChildFolderPath == folder);

        std::string label = std::string(ICON_FA_FOLDER) + "  " + folder;

        if (ImGui::Selectable(label.c_str(), selected))
        {
            m_ctx.selectedAssetChildFolderPath = folder;
        }
    }

    ImGui::NextColumn();

    // right column: content of selected folder
    if (!m_ctx.selectedAssetChildFolderPath.empty())
    {
        std::string fullPath = std::string(GAME_ASSETS_DIR) + "/" + m_ctx.selectedAssetChildFolderPath;

        auto contents = getDirectoryContents(fullPath);

        for (const auto &item : contents)
        {
            std::string label = std::string(getIconForItem(item)) + "  " + item.name;
            ImGui::Selectable(label.c_str());
        }
    }

    ImGui::Columns(1);
    ImGui::End();
}
