#include "core/AnimationMap.h"
#include "core/AnimationPlayer.h"
#include "core/AssetManager.h"
#include "core/IconsFontAwesome4.h"
#include "core/ImGuiWrapper.h"
#include "core/SpriteSheet.h"

#include "editor/AssetsPanel.h"
#include "editor/Constants.h"
#include "editor/DragNDropPayloads.h"

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

    if (item.extension == ".yaml" || item.extension == ".yml")
    {
        return ICON_FA_FILE_VIDEO_O;
    }

    if (item.extension == ".ttf")
    {
        return ICON_FA_FONT;
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
        drawAnimationPanel();
    }
    drawBoundTextures();
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
        else
        {
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
                ImGui::DockBuilderDockWindow("Animations##AssetsPanel", right);
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

    std::string selectedFolderPath = std::string(GAME_ASSETS_DIR) + m_ctx.selectedAssetChildFolderPath;
    auto contents = getDirectoryContents(selectedFolderPath);

    for (const auto &item : contents)
    {
        std::string currentContentFullPath = selectedFolderPath + "/" + item.nameWithExtention;

        if (item.extension == ".png" || item.extension == ".jpg")
        {
            std::string jsonPath = selectedFolderPath + "/" + item.nameWithoutExtention + ".json";
            auto spriteSheet = AssetManager::instance().getSpriteSheet(jsonPath);

            if (ImGuiWrapper::ImageButtonFixedHeight(spriteSheet->fullSprite))
            {
                m_ctx.selectedTextureJsonPath = jsonPath;
                m_rebuildDock = true;
            }
        }
        else if (item.extension == ".yaml" || item.extension == ".yml")
        {
            std::string label = std::string(getIconForItem(item)) + "  " + item.nameWithoutExtention;
            if (ImGui::Selectable(label.c_str(), m_ctx.selectedScenePath == currentContentFullPath))
            {
                m_ctx.selectedScenePath = currentContentFullPath;
            }
        }
        else if (item.extension == ".ttf")
        {
            std::string label = std::string(getIconForItem(item)) + "  " + item.nameWithoutExtention;
            if (ImGui::Selectable(label.c_str(), m_ctx.selectedFontPath == currentContentFullPath))
            {
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

    ImGui::End();
}

void AssetsPanel::drawAnimationPanel()
{
    ImGui::Begin("Animations##AssetsPanel");

    if (m_ctx.selectedTextureJsonPath.empty())
    {
        ImGui::Text("First select a texture");
        ImGui::End();
        return;
    }

    auto path = getFilePath(m_ctx.selectedTextureJsonPath);
    std::shared_ptr<AnimationMap> animationMap = AssetManager::instance().getAnimationMap(path);
    ImTextureID texId = animationMap->getTexture()->getTextureId();

    // create animation player for each animation
    static std::unordered_map<std::string, AnimationPlayer> animationPlayerMap;

    int imguiId = 0;
    for (auto &[animName, animation] : animationMap->getAnimations())
    {
        auto it = animationPlayerMap.find(animName);
        if (it == animationPlayerMap.end())
        {
            animationPlayerMap[animName] = AnimationPlayer{animation};
        }
        animationPlayerMap[animName].update();

        std::array<glm::vec2, 4> texCoord =
            animationPlayerMap[animName].getCurrentFrame().sprite.getNormalizedTextureCoordinates();
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

    ImGui::End();
}

void AssetsPanel::drawBoundTextures()
{
    ImGui::Begin("Bound Textures##AssetsPanel");

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

    ImGui::End();
}
