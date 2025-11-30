#include "editor/AssetsPanel.h"
#include "editor/SpritePayload.h"

#include "core/AssetManager.h"
#include "core/IconsFontAwesome4.h"
#include "core/ImGuiWrapper.h"
#include "core/SpriteSheet.h"

#include "util/PathUtils.h"

#include <filesystem>
#include <imgui.h>
#include <imgui/imgui_internal.h>
#include <memory>

namespace fs = std::filesystem;

struct AssetItem
{
    std::string nameWithExtention;
    std::string nameWithoutExtention;
    bool isDirectory;
    std::string extension;
};

inline std::vector<AssetItem> getDirectoryContents(const std::string &path)
{
    std::vector<AssetItem> items;

    for (const auto &entry : fs::directory_iterator(path))
    {
        AssetItem item;
        item.nameWithExtention = entry.path().filename().string();
        item.nameWithoutExtention = entry.path().stem().string();
        item.isDirectory = entry.is_directory();
        item.extension = entry.path().extension().string();
        items.push_back(item);
    }

    return items;
}

inline const char *getIconForItem(const AssetItem &item)
{
    if (item.isDirectory)
    {
        return ICON_FA_FOLDER;
    }

    // file extension icons
    if (item.extension == ".png" || item.extension == ".jpg")
    {
        return ICON_FA_FILE_IMAGE_O;
    }

    if (item.extension == ".json" || item.extension == ".txt")
    {
        return ICON_FA_FILE_TEXT_O;
    }

    if (item.extension == ".wav" || item.extension == ".ogg")
    {
        return ICON_FA_FILE_AUDIO_O;
    }

    // fallback
    return ICON_FA_FILE_O;
}

// TODO: DO WE NEED THIS?
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
    buildAssetsDockspace();

    drawFoldersPanel();

    if (!m_ctx.selectedAssetChildFolderPath.empty())
    {
        drawContentPanel();
    }

    if (!m_ctx.selectedTextureJsonPath.empty())
    {
        drawSpritePanel();
    }
}

void AssetsPanel::buildAssetsDockspace()
{
    ImGui::Begin("Assets");

    ImGuiID dockspaceId = ImGui::GetID("AssetsDockspace");

    // Rebuild if first time OR dock node missing
    if (m_rebuildDock || !ImGui::DockBuilderGetNode(dockspaceId))
    {
        m_rebuildDock = false;

        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetContentRegionAvail());

        if (m_ctx.selectedAssetChildFolderPath.empty())
        {
            ImGui::DockBuilderDockWindow("AssetsFolders", dockspaceId);
        }
        else {
            if (m_ctx.selectedTextureJsonPath.empty())
            {
                ImGuiID left, right;
                ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.25f, &left, &right);

                ImGui::DockBuilderDockWindow("AssetsFolders", left);
                ImGui::DockBuilderDockWindow("AssetsContent", right);
            }
            else
            {
                ImGuiID left, center, right;
                ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.25f, &left, &center);
                ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.40f, &center, &right);

                ImGui::DockBuilderDockWindow("AssetsFolders", left);
                ImGui::DockBuilderDockWindow("AssetsContent", center);
                ImGui::DockBuilderDockWindow("Sprites##AssetsPanel", right);
            }
        }

        ImGui::DockBuilderFinish(dockspaceId);
    }

    ImGui::DockSpace(dockspaceId, ImVec2(0, 0));
    ImGui::End();
}

void AssetsPanel::drawFoldersPanel()
{
    ImGui::Begin("AssetsFolders");

    auto folders = getFoldersInDirectory(GAME_ASSETS_DIR);
    for (const auto &folder : folders)
    {
        bool selected = (m_ctx.selectedAssetChildFolderPath == folder);
        std::string label = std::string(ICON_FA_FOLDER) + "  " + folder;

        if (ImGui::Selectable(label.c_str(), selected))
        {
            m_ctx.selectedAssetChildFolderPath = folder;
            m_ctx.selectedTextureJsonPath.clear();
            m_rebuildDock = true;
        }
    }

    ImGui::End();
}

void AssetsPanel::drawContentPanel()
{
    ImGui::Begin("AssetsContent");

    std::string fullPath = std::string(GAME_ASSETS_DIR) + m_ctx.selectedAssetChildFolderPath;
    auto contents = getDirectoryContents(fullPath);

    for (const auto &item : contents)
    {
        // image files
        if (item.extension == ".png" || item.extension == ".jpg")
        {
            std::string jsonPath = fullPath + "/" + item.nameWithoutExtention + ".json";
            auto spriteSheet = AssetManager::instance().getSpriteSheet(jsonPath);

            if (ImGuiWrapper::ImageButtonFixedHeight(spriteSheet->fullSprite))
            {
                m_ctx.selectedTextureJsonPath = jsonPath;
                m_rebuildDock = true;
            }
        }
        else if (item.extension != ".json")
        {
            std::string label = std::string(getIconForItem(item)) + "  " + item.nameWithoutExtention;
            ImGui::Selectable(label.c_str());
        }
    }

    ImGui::End();
}

void AssetsPanel::drawSpritePanel()
{
    ImGui::Begin("Sprites##AssetsPanel");

    std::shared_ptr<SpriteSheet> spriteSheet = AssetManager::instance().getSpriteSheet(m_ctx.selectedTextureJsonPath);
    std::shared_ptr<Texture> texture = spriteSheet->getTexture();

    for (Sprite &sprite : spriteSheet->getSprites())
    {
        float imgButtonWidth = 64;
        float imgButtonHeight = 64;
        std::array<glm::vec2, 4> textureCoordinates = sprite.getNormalizedTextureCoordinates();
        ImTextureID texId = (ImTextureID)(uintptr_t)texture->getTextureId();

        ImGui::PushID(sprite.getId());
        if (ImGui::ImageButton("", texId, ImVec2(imgButtonWidth, imgButtonHeight),
                ImVec2(textureCoordinates[0].x,
                    textureCoordinates[0].y), // uv0 = top-left
                ImVec2(textureCoordinates[2].x,
                    textureCoordinates[2].y), // uv1 = bottom-right
                ImVec4(0.0f, 0.0f, 0.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f)))
        {
            m_ctx.selectedSprite = sprite;
        }
        ImGui::PopID();

        // Drag & drop source
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            m_ctx.selectedSprite = sprite;
            int id = sprite.getId();

            // payload will be ignored, we'll use m_ctx.selecteSprite in drop target
            // but imgui requires a payload it for drag and drop to work
            ImGui::SetDragDropPayload("SPRITE", &id, sizeof(int));
            ImGui::Text("Dragging sprite %d", id);
            ImGui::EndDragDropSource();
        }

        // handle wrapping
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();

        float windowX2 = windowPos.x + windowSize.x;
        ImVec2 lastSpritePosition = ImGui::GetItemRectMax();
        float lastSpriteX2 = lastSpritePosition.x;
        float nextButtonX2 = lastSpriteX2 + imgButtonWidth;
        if (nextButtonX2 < windowX2)
        {
            ImGui::SameLine();
        }
    }

    ImGui::End();
}
