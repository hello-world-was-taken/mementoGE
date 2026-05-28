// Enable ImGui math operators for this translation unit before including imgui.h
#define IMGUI_DEFINE_MATH_OPERATORS

#include "core/AnimationMap.h"
#include "core/AnimationPlayer.h"
#include "core/AssetManager.h"
#include "core/GlResourceManager.h"
#include "core/IconsFontAwesome4.h"
#include "core/ImGuiWrapper.h"
#include "core/SpriteSheet.h"

#include "editor/Constants.h"
#include "editor/DragNDropPayloads.h"
#include "editor/panels/AssetsPanel.h"

#include "util/LogChannels.h"
#include "util/PathUtils.h"

#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <fstream>
#include <imgui.h>
#include <imgui/imgui_internal.h>
#include <memory>
#include <sawyer/Logger.h>
#include <system_error>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

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

    std::error_code error;
    for (const auto &entry : fs::directory_iterator(path, error))
    {
        AssetItem item;
        item.nameWithExtention = entry.path().filename().string();
        item.nameWithoutExtention = entry.path().stem().string();
        item.isDirectory = entry.is_directory();
        item.extension = entry.path().extension().string();
        items.push_back(item);
    }

    std::sort(items.begin(),
        items.end(),
        [](const AssetItem &left, const AssetItem &right)
        {
            if (left.isDirectory != right.isDirectory)
            {
                return left.isDirectory;
            }
            return left.nameWithExtention < right.nameWithExtention;
        });

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

    if (item.extension == ".yaml" || item.extension == ".yml")
    {
        return ICON_FA_FILE_VIDEO_O;
    }

    if (item.extension == ".ttf")
    {
        return ICON_FA_FONT;
    }

    if (item.extension == ".wav")
    {
        return ICON_FA_FILE_AUDIO_O;
    }

    // fallback
    return ICON_FA_FILE_O;
}

enum class YamlAssetKind
{
    Unknown,
    Scene,
    Model,
};

YamlAssetKind getYamlAssetKind(const fs::path &path)
{
    struct CachedKind
    {
        fs::file_time_type lastWriteTime;
        YamlAssetKind kind = YamlAssetKind::Unknown;
    };
    static std::unordered_map<std::string, CachedKind> cache;

    std::error_code error;
    const fs::file_time_type lastWriteTime = fs::last_write_time(path, error);
    const std::string key = path.lexically_normal().string();
    if (!error)
    {
        const auto cached = cache.find(key);
        if (cached != cache.end() && cached->second.lastWriteTime == lastWriteTime)
        {
            return cached->second.kind;
        }
    }

    YamlAssetKind kind = YamlAssetKind::Unknown;
    try
    {
        const YAML::Node root = YAML::LoadFile(key);
        if (root.begin() != root.end())
        {
            const YAML::Node contents = root.begin()->second;
            if (contents["GameObjects"])
            {
                kind = YamlAssetKind::Scene;
            }
            else if (contents["EntityInfo"])
            {
                kind = YamlAssetKind::Model;
            }
        }
    }
    catch (const std::exception &)
    {
    }
    if (!error)
    {
        cache[key] = {lastWriteTime, kind};
    }
    return kind;
}

bool isTextureImage(const fs::path &path)
{
    const std::string extension = path.extension().string();
    return extension == ".png" || extension == ".jpg" || extension == ".jpeg";
}

bool drawAssetImageButton(const std::shared_ptr<Texture> &texture,
    const ImVec2 &uv0,
    const ImVec2 &uv1,
    float sourceWidth,
    float sourceHeight,
    bool selected)
{
    if (!texture || sourceWidth <= 0.0f || sourceHeight <= 0.0f)
    {
        return false;
    }

    constexpr float maxWidth = 128.0f;
    constexpr float maxHeight = 96.0f;
    const float scale = std::min(maxWidth / sourceWidth, maxHeight / sourceHeight);
    const ImVec2 previewSize(std::max(1.0f, sourceWidth * scale), std::max(1.0f, sourceHeight * scale));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (maxWidth - previewSize.x) * 0.5f);
    if (selected)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Header));
    }
    const bool clicked = ImGui::ImageButton(
        "##ImageAsset", texture->getTextureId(), previewSize, uv0, uv1, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    if (selected)
    {
        ImGui::PopStyleColor();
    }
    return clicked;
}

std::vector<fs::path> getAssetDeletionTargets(const fs::path &assetPath)
{
    std::vector<fs::path> targets{assetPath};
    if (isTextureImage(assetPath))
    {
        fs::path metadataPath = assetPath;
        metadataPath.replace_extension(".json");
        std::error_code error;
        if (fs::exists(metadataPath, error) && !error)
        {
            targets.push_back(std::move(metadataPath));
        }
    }
    return targets;
}

bool isInsideDirectory(const fs::path &path, const fs::path &directory)
{
    const fs::path relativePath = fs::weakly_canonical(path).lexically_relative(fs::weakly_canonical(directory));
    if (relativePath.empty() || relativePath == ".")
    {
        return false;
    }

    for (const fs::path &part : relativePath)
    {
        if (part == "..")
        {
            return false;
        }
    }
    return true;
}

AssetsPanel::AssetsPanel(EditorContext &ctx) : EditorPanel{ctx}, m_ctx{ctx}
{
}

AssetsPanel::~AssetsPanel()
{
}

void AssetsPanel::requestAssetDeletion(const fs::path &assetPath)
{
    std::error_code error;
    if (assetPath.empty() || fs::is_directory(assetPath, error) || error)
    {
        return;
    }

    m_pendingDeletionPath = assetPath;
    m_deleteAssetError.clear();
    m_openDeleteAssetPopup = true;
}

bool AssetsPanel::deletePendingAsset()
{
    try
    {
        const fs::path gameAssetsDirectory = getGameAssetPath("");
        const std::vector<fs::path> targets = getAssetDeletionTargets(m_pendingDeletionPath);

        for (const fs::path &target : targets)
        {
            if (!isInsideDirectory(target, gameAssetsDirectory) || !fs::is_regular_file(target))
            {
                m_deleteAssetError = "Refusing to delete a path outside the game assets directory.";
                LOG_ERROR(LogChannel::Asset, "{} Target: '{}'", m_deleteAssetError, target.string());
                return false;
            }
        }

        for (const fs::path &target : targets)
        {
            std::error_code error;
            if (!fs::remove(target, error) || error)
            {
                m_deleteAssetError = "Could not delete '" + target.filename().string() + "': " + error.message();
                LOG_ERROR(LogChannel::Asset, "{}", m_deleteAssetError);
                return false;
            }

            const std::string targetPath = target.string();
            AssetManager::instance().forgetAsset(targetPath);
            GlResourceManager::instance().forgetTexture(targetPath);
            m_ctx.sceneByScenePathMap.erase(targetPath);
            m_ctx.sceneHistoryByScenePathMap.erase(targetPath);
            LOG_INFO(LogChannel::Asset, "Deleted game asset '{}'", targetPath);
        }

        const std::string deletedPath = m_pendingDeletionPath.string();
        fs::path deletedMetadataPath = m_pendingDeletionPath;
        deletedMetadataPath.replace_extension(".json");
        if (m_ctx.selectedTextureJsonPath == deletedPath ||
            (isTextureImage(m_pendingDeletionPath) && m_ctx.selectedTextureJsonPath == deletedMetadataPath.string()))
        {
            m_ctx.selectedTextureJsonPath.clear();
        }
        if (m_ctx.selectedFontPath == deletedPath)
        {
            m_ctx.selectedFontPath.clear();
        }
        if (m_ctx.selectedScenePath == deletedPath)
        {
            m_ctx.selectedScenePath.clear();
            m_ctx.selectedObjects.clear();
            m_ctx.selectedGameObjectsDragOffset.clear();
            m_ctx.markEditorWorkspaceDirty();
        }

        m_selectedAssetPath.clear();
        m_pendingDeletionPath.clear();
        m_deleteAssetError.clear();
        return true;
    }
    catch (const fs::filesystem_error &error)
    {
        m_deleteAssetError = error.what();
        LOG_ERROR(LogChannel::Asset, "Could not delete game asset: {}", error.what());
        return false;
    }
}

void AssetsPanel::drawDeleteAssetPopup()
{
    if (m_openDeleteAssetPopup)
    {
        ImGui::OpenPopup("Delete Asset?");
        m_openDeleteAssetPopup = false;
    }

    if (!ImGui::BeginPopupModal("Delete Asset?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    ImGui::TextWrapped("Permanently delete this game asset?");
    ImGui::BulletText("%s", m_pendingDeletionPath.filename().string().c_str());

    fs::path metadataPath = m_pendingDeletionPath;
    metadataPath.replace_extension(".json");
    std::error_code metadataError;
    if (isTextureImage(m_pendingDeletionPath) && fs::exists(metadataPath, metadataError) && !metadataError)
    {
        ImGui::TextWrapped("Its sprite and animation metadata will also be deleted:");
        ImGui::BulletText("%s", metadataPath.filename().string().c_str());
    }

    const bool deletingActiveScene =
        m_ctx.isRuntimeActive() && m_ctx.selectedScenePath == m_pendingDeletionPath.string();
    if (deletingActiveScene)
    {
        ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "Stop play mode before deleting the active scene.");
    }
    if (!m_deleteAssetError.empty())
    {
        ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "%s", m_deleteAssetError.c_str());
    }

    ImGui::BeginDisabled(deletingActiveScene);
    if (ImGui::Button("Delete"))
    {
        if (deletePendingAsset())
        {
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        m_pendingDeletionPath.clear();
        m_deleteAssetError.clear();
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void AssetsPanel::draw()
{
    const bool wasOpen = m_ctx.workspaceState.showAssetsPanel;
    if (m_ctx.workspaceState.showAssetsPanel)
    {
        if (ImGuiWrapper::BeginPanel("Assets", &m_ctx.workspaceState.showAssetsPanel))
        {
            if (m_ctx.selectedTextureJsonPath.empty())
            {
                drawContentPanel();
            }
            else if (ImGui::BeginTable("AssetsWithPreviews",
                         2,
                         ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV |
                             ImGuiTableFlags_SizingStretchProp))
            {
                ImGui::TableSetupColumn("Browser", ImGuiTableColumnFlags_WidthStretch, 0.7f);
                ImGui::TableSetupColumn("Previews", ImGuiTableColumnFlags_WidthStretch, 0.3f);
                ImGui::TableNextColumn();
                drawContentPanel();
                ImGui::TableNextColumn();

                if (!m_ctx.selectedTextureJsonPath.empty())
                {
                    const float regionHeight = std::max(100.0f,
                        (ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ItemSpacing.y) * 0.5f);
                    ImGui::TextUnformatted("Sprites");
                    ImGui::BeginChild("##SpriteAssets", ImVec2(0.0f, regionHeight), true);
                    drawSpritePanel();
                    ImGui::EndChild();

                    ImGui::TextUnformatted("Animations");
                    ImGui::BeginChild("##AnimationAssets", ImVec2(0.0f, 0.0f), true);
                    drawAnimationPanel();
                    ImGui::EndChild();
                }
                ImGui::EndTable();
            }
        }
        ImGuiWrapper::EndPanel();
    }
    if (wasOpen != m_ctx.workspaceState.showAssetsPanel)
    {
        m_ctx.markEditorWorkspaceDirty();
    }

    drawBoundTextures();
}

// FIXME: this function is doing a lot of things.
void AssetsPanel::drawContentPanel()
{
    const fs::path assetsRoot = getGameAssetPath("").lexically_normal();
    fs::path relativeFolder = fs::path(m_ctx.selectedAssetChildFolderPath).lexically_normal();
    if (relativeFolder == ".")
    {
        relativeFolder.clear();
    }
    fs::path selectedFolder = (assetsRoot / relativeFolder).lexically_normal();
    std::error_code selectedFolderError;
    if (!fs::is_directory(selectedFolder, selectedFolderError) || selectedFolderError ||
        (!relativeFolder.empty() && !isInsideDirectory(selectedFolder, assetsRoot)))
    {
        relativeFolder.clear();
        selectedFolder = assetsRoot;
        m_ctx.selectedAssetChildFolderPath.clear();
    }

    auto navigateTo = [&](const fs::path &relativePath)
    {
        m_ctx.selectedAssetChildFolderPath = relativePath.empty() ? std::string{} : relativePath.generic_string();
        m_ctx.selectedTextureJsonPath.clear();
        m_selectedAssetPath.clear();
    };

    if (!ImGui::BeginTable("AssetsBrowserSplit",
            2,
            ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp |
                ImGuiTableFlags_ScrollY,
            ImVec2(0.0f, 0.0f)))
    {
        return;
    }
    ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 260.0f);
    ImGui::TableSetupColumn("Contents", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableNextColumn();

    std::function<void(const fs::path &, const fs::path &, const std::string &, bool)> drawFolderNode;
    drawFolderNode = [&](const fs::path &absolutePath,
                         const fs::path &relativePath,
                         const std::string &label,
                         bool isRoot)
    {
        const std::vector<AssetItem> children = getDirectoryContents(absolutePath.string());
        const bool hasFolders = std::any_of(children.begin(),
            children.end(),
            [](const AssetItem &item)
            {
                return item.isDirectory;
            });
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (relativeFolder == relativePath)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        if (isRoot)
        {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }
        if (!hasFolders)
        {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        ImGui::PushID(absolutePath.string().c_str());
        const bool open = ImGui::TreeNodeEx("##Folder", flags, "%s  %s", ICON_FA_FOLDER, label.c_str());
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            navigateTo(relativePath);
        }
        if (open && hasFolders)
        {
            for (const AssetItem &child : children)
            {
                if (child.isDirectory)
                {
                    drawFolderNode(absolutePath / child.nameWithExtention,
                        relativePath / child.nameWithExtention,
                        child.nameWithExtention,
                        false);
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    };
    drawFolderNode(assetsRoot, {}, "Assets", true);

    ImGui::TableNextColumn();

    ImGui::BeginDisabled(relativeFolder.empty());
    if (ImGui::Button(ICON_FA_ARROW_LEFT "##AssetsUp"))
    {
        navigateTo(relativeFolder.parent_path());
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_HOME "  Assets"))
    {
        navigateTo({});
    }
    fs::path breadcrumb;
    for (const fs::path &part : relativeFolder)
    {
        breadcrumb /= part;
        ImGui::SameLine();
        ImGui::TextUnformatted(">");
        ImGui::SameLine();
        ImGui::PushID(breadcrumb.generic_string().c_str());
        if (ImGui::Button(part.string().c_str()))
        {
            navigateTo(breadcrumb);
        }
        ImGui::PopID();
    }

    ImGui::BeginDisabled(m_selectedAssetPath.empty());
    if (ImGuiWrapper::Button("Delete Asset"))
    {
        requestAssetDeletion(m_selectedAssetPath);
    }
    ImGui::EndDisabled();
    if (!m_selectedAssetPath.empty() && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        !ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_Delete, false))
    {
        requestAssetDeletion(m_selectedAssetPath);
    }
    ImGui::Separator();

    const fs::path category = relativeFolder.empty() ? fs::path{} : *relativeFolder.begin();
    if (relativeFolder == "scenes")
    {
        static char newSceneName[64] = "";

        ImGui::InputText("Scene Name", newSceneName, IM_ARRAYSIZE(newSceneName));
        ImGui::SameLine();
        if (ImGuiWrapper::Button("Create Scene") && newSceneName[0] != '\0')
        {
            std::string sceneName = newSceneName;

            // Build the full path for the new scene asset.
            std::string scenePath = (selectedFolder / (sceneName + ".yaml")).string();

            if (!fs::exists(scenePath))
            {
                // Insert a fresh Scene into the editor context and select it.
                m_ctx.sceneByScenePathMap.insert_or_assign(scenePath, Scene{std::move(sceneName)});
                m_ctx.selectedScenePath = scenePath;
                m_ctx.markEditorWorkspaceDirty();

                // Persist the new scene to disk using the existing helper.
                m_ctx.serializaActiveScene();

                // Clear name for next creation.
                newSceneName[0] = '\0';
            }
        }

        ImGui::Separator();
    }
    else if (category == "audio")
    {
        if (ImGuiWrapper::Button("Import Audio"))
        {
            ImGuiFileDialog::Instance()->OpenDialog("ImportAudioFile", "Select Audio", ".wav,.ogg");
        }

        if (ImGuiFileDialog::Instance()->Display("ImportAudioFile"))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                std::string src = ImGuiFileDialog::Instance()->GetFilePathName();
                std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
                fs::path destPath = selectedFolder / fileName;

                try
                {
                    fs::create_directories(destPath.parent_path());
                    fs::copy_file(src, destPath, fs::copy_options::update_existing);
                }
                catch (const fs::filesystem_error &e)
                {
                    std::cerr << "Failed to copy audio: " << e.what() << std::endl;
                }
            }

            ImGuiFileDialog::Instance()->Close();
        }

        ImGui::Separator();
    }
    else if (category == "fonts")
    {
        if (ImGuiWrapper::Button("Import Font"))
        {
            ImGuiFileDialog::Instance()->OpenDialog("ImportFontFile", "Select Font", ".ttf");
        }

        if (ImGuiFileDialog::Instance()->Display("ImportFontFile"))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                std::string src = ImGuiFileDialog::Instance()->GetFilePathName();
                std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
                fs::path destPath = selectedFolder / fileName;

                try
                {
                    fs::create_directories(destPath.parent_path());
                    fs::copy_file(src, destPath, fs::copy_options::update_existing);
                }
                catch (const fs::filesystem_error &e)
                {
                    std::cerr << "Failed to copy font: " << e.what() << std::endl;
                }
            }

            ImGuiFileDialog::Instance()->Close();
        }

        ImGui::Separator();
    }
    else if (category == "texture")
    {
        if (ImGuiWrapper::Button("Import Texture"))
        {
            ImGuiFileDialog::Instance()->OpenDialog("ImportTextureFile", "Select Texture", ".png,.jpg,.jpeg");
        }

        if (ImGuiFileDialog::Instance()->Display("ImportTextureFile"))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                std::string src = ImGuiFileDialog::Instance()->GetFilePathName();
                std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
                fs::path destPath = selectedFolder / fileName;

                try
                {
                    fs::create_directories(destPath.parent_path());
                    fs::copy_file(src, destPath, fs::copy_options::update_existing);

                    // Create a matching empty spritesheet JSON if it does not exist yet.
                    fs::path jsonPath = destPath.parent_path() / (destPath.stem().string() + ".json");
                    if (!fs::exists(jsonPath))
                    {
                        std::ofstream jsonFile(jsonPath);
                        if (jsonFile.is_open())
                        {
                            // Match existing format: meta.texture, frames object, animations object.
                            jsonFile << "{\n";
                            jsonFile << "  \"meta\": { \"texture\": \"" << fileName << "\" },\n";
                            jsonFile << "  \"frames\": {},\n";
                            jsonFile << "  \"animations\": {}\n";
                            jsonFile << "}\n";
                        }
                    }
                }
                catch (const fs::filesystem_error &e)
                {
                    std::cerr << "Failed to copy texture: " << e.what() << std::endl;
                }
            }

            ImGuiFileDialog::Instance()->Close();
        }

        ImGui::Separator();
    }
    auto contents = getDirectoryContents(selectedFolder.string());
    contents.erase(std::remove_if(contents.begin(),
                       contents.end(),
                       [&](const AssetItem &item)
                       {
                           if (item.extension != ".json")
                           {
                               return false;
                           }
                           for (const char *extension : {".png", ".jpg", ".jpeg"})
                           {
                               if (fs::exists(selectedFolder / (item.nameWithoutExtention + extension)))
                               {
                                   return true;
                               }
                           }
                           return false;
                       }),
        contents.end());
    auto clearTextureDetails = [&]
    {
        if (!m_ctx.selectedTextureJsonPath.empty())
        {
            m_ctx.selectedTextureJsonPath.clear();
        }
    };

    constexpr float assetCellWidth = 150.0f;
    constexpr float assetCellHeight = 142.0f;
    const float gridRight = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;

    for (const auto &item : contents)
    {
        const fs::path currentAssetPath = selectedFolder / item.nameWithExtention;
        const std::string currentContentFullPath = currentAssetPath.string();
        const bool selected = m_selectedAssetPath == currentAssetPath;
        ImGui::PushID(currentContentFullPath.c_str());
        const ImVec2 cellStart = ImGui::GetCursorScreenPos();
        ImGui::BeginGroup();
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + assetCellWidth);

        auto drawDeleteContextMenu = [&]()
        {
            if (ImGui::BeginPopupContextItem("AssetContextMenu"))
            {
                if (ImGui::MenuItem("Delete..."))
                {
                    m_selectedAssetPath = currentAssetPath;
                    requestAssetDeletion(currentAssetPath);
                }
                ImGui::EndPopup();
            }
        };

        if (item.isDirectory)
        {
            const std::string label = std::string(ICON_FA_FOLDER) + "  " + item.nameWithExtention;
            if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_None, ImVec2(assetCellWidth, 0.0f)))
            {
                navigateTo(relativeFolder / item.nameWithExtention);
            }
        }
        else if (item.extension == ".png" || item.extension == ".jpg" || item.extension == ".jpeg")
        {
            const std::string jsonPath = (selectedFolder / (item.nameWithoutExtention + ".json")).string();
            std::shared_ptr<SpriteSheet> spriteSheet;
            if (fs::exists(jsonPath))
            {
                spriteSheet = AssetManager::instance().getSpriteSheet(jsonPath);
            }
            bool clicked = false;
            bool renderedPreview = false;
            if (spriteSheet)
            {
                const std::array<glm::vec2, 4> coordinates = spriteSheet->fullSprite.getNormalizedTextureCoordinates();
                clicked = drawAssetImageButton(spriteSheet->fullSprite.texture,
                    ImVec2(coordinates[0].x, coordinates[0].y),
                    ImVec2(coordinates[2].x, coordinates[2].y),
                    spriteSheet->fullSprite.width,
                    spriteSheet->fullSprite.height,
                    selected);
                renderedPreview = true;
            }
            else
            {
                const std::shared_ptr<Texture> texture = GlResourceManager::instance().getTexture(currentContentFullPath);
                if (texture && texture->getWidth() > 0 && texture->getHeight() > 0)
                {
                    clicked = drawAssetImageButton(texture,
                        ImVec2(0.0f, 0.0f),
                        ImVec2(1.0f, 1.0f),
                        static_cast<float>(texture->getWidth()),
                        static_cast<float>(texture->getHeight()),
                        selected);
                    renderedPreview = true;
                }
                else
                {
                    const std::string label = std::string(ICON_FA_FILE_IMAGE_O) + "  " + item.nameWithoutExtention;
                    clicked = ImGui::Selectable(
                        label.c_str(), selected, ImGuiSelectableFlags_None, ImVec2(assetCellWidth, 0.0f));
                }
            }
            if (clicked)
            {
                m_selectedAssetPath = currentAssetPath;
                const std::string selectedMetadataPath = fs::exists(jsonPath) ? jsonPath : std::string{};
                if (m_ctx.selectedTextureJsonPath != selectedMetadataPath)
                {
                    m_ctx.selectedTextureJsonPath = selectedMetadataPath;
                }
            }
            drawDeleteContextMenu();
            if (renderedPreview)
            {
                ImGui::TextWrapped("%s", item.nameWithoutExtention.c_str());
            }
        }
        else if (item.extension == ".yaml" || item.extension == ".yml")
        {
            const YamlAssetKind kind = getYamlAssetKind(currentAssetPath);
            const char *icon = kind == YamlAssetKind::Scene ? ICON_FA_GAMEPAD
                                                            : (kind == YamlAssetKind::Model ? ICON_FA_CUBE
                                                                                           : getIconForItem(item));
            std::string label = std::string(icon) + "  " + item.nameWithoutExtention;

            if (kind == YamlAssetKind::Scene)
            {
                if (ImGui::Selectable(label.c_str(),
                        m_ctx.selectedScenePath == currentContentFullPath,
                        ImGuiSelectableFlags_None,
                        ImVec2(assetCellWidth, 0.0f)))
                {
                    clearTextureDetails();
                    m_selectedAssetPath = currentAssetPath;
                    m_ctx.selectedScenePath = currentContentFullPath;
                    m_ctx.markEditorWorkspaceDirty();
                }
            }
            else if (kind == YamlAssetKind::Model)
            {
                if (ImGui::Selectable(
                        label.c_str(), selected, ImGuiSelectableFlags_None, ImVec2(assetCellWidth, 0.0f)))
                {
                    clearTextureDetails();
                    m_selectedAssetPath = currentAssetPath;
                }

                // GameObject models remain draggable regardless of their containing folder.
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    ModelPayload payload{};
                    strncpy(payload.filePath, currentContentFullPath.c_str(), sizeof(payload.filePath) - 1);

                    ImGui::SetDragDropPayload("MODEL", &payload, sizeof(ModelPayload));
                    ImGui::Text("Dragging model %s", currentContentFullPath.c_str());
                    ImGui::EndDragDropSource();
                }
            }
            else
            {
                // Other YAML assets: just list them for now.
                if (ImGui::Selectable(
                        label.c_str(), selected, ImGuiSelectableFlags_None, ImVec2(assetCellWidth, 0.0f)))
                {
                    clearTextureDetails();
                    m_selectedAssetPath = currentAssetPath;
                }
            }
            drawDeleteContextMenu();
        }
        else if (item.extension == ".ttf")
        {
            std::string label = std::string(getIconForItem(item)) + "  " + item.nameWithoutExtention;
            if (ImGui::Selectable(label.c_str(),
                    m_ctx.selectedFontPath == currentContentFullPath,
                    ImGuiSelectableFlags_None,
                    ImVec2(assetCellWidth, 0.0f)))
            {
                clearTextureDetails();
                m_selectedAssetPath = currentAssetPath;
                m_ctx.selectedFontPath = currentContentFullPath;
            }

            // Drag & drop source
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                // TODO: using m_ctx.selectedFontPath requires an initial click so restoring to using
                // currentContentFullPath. Verify
                FontPayload payload{};
                strncpy(payload.fontPath, currentContentFullPath.c_str(), sizeof(payload.fontPath) - 1);

                ImGui::SetDragDropPayload("FONT", &payload, sizeof(FontPayload));
                ImGui::Text("Dragging font %s", currentContentFullPath.c_str());
                ImGui::EndDragDropSource();
            }
            drawDeleteContextMenu();
        }
        else if (item.extension == ".wav" || item.extension == ".ogg")
        {
            std::string label = std::string(getIconForItem(item)) + "  " + item.nameWithoutExtention;

            if (ImGui::Selectable(
                    label.c_str(), selected, ImGuiSelectableFlags_None, ImVec2(assetCellWidth, 0.0f)))
            {
                clearTextureDetails();
                m_selectedAssetPath = currentAssetPath;
            }

            // Drag & drop source
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                AudioPayload payload{};
                strncpy(payload.filePath, currentContentFullPath.c_str(), sizeof(payload.filePath) - 1);

                ImGui::SetDragDropPayload("AUDIO", &payload, sizeof(AudioPayload));
                ImGui::Text("Dragging audio %s", currentContentFullPath.c_str());

                ImGui::EndDragDropSource();
            }
            drawDeleteContextMenu();
        }
        else if (item.extension == ".json")
        {
            fs::path imagePath = currentAssetPath;
            imagePath.replace_extension(".png");
            fs::path jpgImagePath = currentAssetPath;
            jpgImagePath.replace_extension(".jpg");
            fs::path jpegImagePath = currentAssetPath;
            jpegImagePath.replace_extension(".jpeg");
            if (!fs::exists(imagePath) && !fs::exists(jpgImagePath) && !fs::exists(jpegImagePath))
            {
                const std::string label = std::string(ICON_FA_FILE_TEXT_O) + "  " + item.nameWithoutExtention;
                if (ImGui::Selectable(
                        label.c_str(), selected, ImGuiSelectableFlags_None, ImVec2(assetCellWidth, 0.0f)))
                {
                    clearTextureDetails();
                    m_selectedAssetPath = currentAssetPath;
                }
                drawDeleteContextMenu();
            }
        }
        else
        {
            std::string label = std::string(getIconForItem(item)) + "  " + item.nameWithoutExtention;
            if (ImGui::Selectable(
                    label.c_str(), selected, ImGuiSelectableFlags_None, ImVec2(assetCellWidth, 0.0f)))
            {
                clearTextureDetails();
                m_selectedAssetPath = currentAssetPath;
            }
            drawDeleteContextMenu();
        }

        ImGui::PopTextWrapPos();
        const float usedHeight = ImGui::GetCursorScreenPos().y - cellStart.y;
        if (usedHeight < assetCellHeight)
        {
            ImGui::Dummy(ImVec2(assetCellWidth, assetCellHeight - usedHeight));
        }
        ImGui::EndGroup();
        ImGui::PopID();

        const float nextCellRight = ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + assetCellWidth;
        if (nextCellRight <= gridRight)
        {
            ImGui::SameLine();
        }
    }

    ImGui::EndTable();

    drawDeleteAssetPopup();
}

void AssetsPanel::drawSpritePanel()
{
    std::shared_ptr<SpriteSheet> spriteSheet = AssetManager::instance().getSpriteSheet(m_ctx.selectedTextureJsonPath);
    if (!spriteSheet)
    {
        ImGui::TextWrapped("Could not load the selected sprite sheet.");
        return;
    }
    std::shared_ptr<Texture> texture = spriteSheet->getTexture();

    for (Sprite &sprite : spriteSheet->getSprites())
    {
        float imgButtonWidth = 64;
        float imgButtonHeight = 64;
        std::array<glm::vec2, 4> textureCoordinates = sprite.getNormalizedTextureCoordinates();
        ImTextureID texId = (ImTextureID)(uintptr_t)texture->getTextureId();

        ImGui::PushID(sprite.getId());
        if (ImGui::ImageButton("",
                texId,
                ImVec2(imgButtonWidth, imgButtonHeight),
                ImVec2(textureCoordinates[0].x,
                    textureCoordinates[0].y), // uv0 = top-left
                ImVec2(textureCoordinates[2].x,
                    textureCoordinates[2].y), // uv1 = bottom-right
                ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
                ImVec4(1.0f, 1.0f, 1.0f, 1.0f)))
        {
        }
        ImGui::PopID();

        // Drag & drop source
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            int id = sprite.getId();

            SpritePayload payload{sprite.topLeft, sprite.width, sprite.height, sprite.texture};
            ImGui::SetDragDropPayload("SPRITE", &payload, sizeof(SpritePayload));
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

}

void AssetsPanel::drawAnimationPanel()
{
    if (m_ctx.selectedTextureJsonPath.empty())
    {
        ImGui::Text("First select a texture");
        return;
    }

    fs::path path = m_ctx.selectedTextureJsonPath;
    std::shared_ptr<AnimationMap> animationMap = AssetManager::instance().getAnimationMap(path);
    if (!animationMap || !animationMap->getTexture())
    {
        ImGui::TextWrapped("Could not load the selected animation file.");
        return;
    }
    ImTextureID texId = animationMap->getTexture()->getTextureId();

    // create animation player for each animation
    static std::unordered_map<std::string, AnimationPlayer> animationPlayerMap;

    int imguiId = 0;
    for (auto &[animName, animation] : animationMap->getAnimations())
    {
        const std::string playerKey = m_ctx.selectedTextureJsonPath + "::" + animName;
        auto it = animationPlayerMap.find(playerKey);
        if (it == animationPlayerMap.end())
        {
            animationPlayerMap[playerKey] = AnimationPlayer{animation};
        }
        animationPlayerMap[playerKey].update();

        std::array<glm::vec2, 4> texCoord =
            animationPlayerMap[playerKey].getCurrentFrame().sprite.getNormalizedTextureCoordinates();
        ImVec2 topLeft = ImVec2(texCoord[0].x, texCoord[0].y);
        ImVec2 bottomRight = ImVec2(texCoord[2].x, texCoord[2].y);

        float imgButtonWidth = 64;
        float imgButtonHeight = 64;

        ImGui::PushID(imguiId);
        if (ImGui::ImageButton("",
                texId,
                ImVec2(imgButtonWidth, imgButtonHeight),
                topLeft,
                bottomRight,
                ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
                ImVec4(1.0f, 1.0f, 1.0f, 1.0f)))
        {
        }
        ImGui::PopID();

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            AnimationPayload payload{};
            strncpy(payload.animationName, animName.c_str(), sizeof(payload.animationName) - 1);
            strncpy(payload.animationJsonPath,
                m_ctx.selectedTextureJsonPath.c_str(),
                sizeof(payload.animationJsonPath) - 1);

            ImGui::SetDragDropPayload("ANIMATION", &payload, sizeof(AnimationPayload));
            ImGui::Text("Dragging animation %s", animName.c_str());
            ImGui::EndDragDropSource();
        }
        imguiId++;

        // handle wrapping
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();

        float windowX2 = windowPos.x + windowSize.x;
        ImVec2 lastSpritePosition = ImGui::GetItemRectMax();
        float lastSpriteX2 = lastSpritePosition.x;
        float nextButtonX2 = lastSpriteX2 + imgButtonWidth;
        if (imguiId < animationMap->getAnimations().size() && nextButtonX2 < windowX2)
        {
            ImGui::SameLine();
        }
    }

}

void AssetsPanel::drawBoundTextures()
{
    ImGuiWrapper::BeginPanel("Bound Textures");

    constexpr float PREVIEW_SIZE = 256.0f;

    // Save previous active texture
    GLint previousActiveTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);

    struct BoundTexture
    {
        int texSlot;        // GL_TEXTURE0, GL_TEXTURE1 ...
        unsigned int texId; // texture id returned by glGenTextures
    };

    static std::vector<BoundTexture> boundTextures;
    boundTextures.clear();

    // Gather bound textures
    for (int texSlot = 0; texSlot < MAX_TEXTURE_SLOTS; ++texSlot)
    {
        glActiveTexture(GL_TEXTURE0 + texSlot);

        GLint tex = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex);

        if (tex != 0)
        {
            boundTextures.push_back({texSlot, (GLuint)tex});
        }
    }

    // Restore active texture
    glActiveTexture(previousActiveTexture);

    static int selectedIndex = 0;
    if (selectedIndex >= (int)boundTextures.size())
    {
        selectedIndex = 0;
    }

    if (!boundTextures.empty())
    {
        std::string previewText = ("Tex Slot " + std::to_string(boundTextures[selectedIndex].texSlot) + " | Tex Id " +
                                   std::to_string(boundTextures[selectedIndex].texId));

        if (ImGui::BeginCombo("Texture Slot", previewText.c_str()))
        {
            for (int i = 0; i < (int)boundTextures.size(); ++i)
            {
                bool selected = (i == selectedIndex);
                std::string label = "Slot " + std::to_string(boundTextures[i].texSlot) + " | Tex " +
                                    std::to_string(boundTextures[i].texId);

                if (ImGui::Selectable(label.c_str(), selected))
                {
                    selectedIndex = i;
                }

                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        ImTextureID texId = boundTextures[selectedIndex].texId;

        ImGui::Text("Preview");
        ImGui::Image(texId,
            ImVec2(PREVIEW_SIZE, PREVIEW_SIZE),
            ImVec2(0.0f, 1.0f), // flip vertically for OpenGL
            ImVec2(1.0f, 0.0f));
    }
    else
    {
        ImGui::TextDisabled("No textures bound");
    }

    ImGuiWrapper::EndPanel();
}
