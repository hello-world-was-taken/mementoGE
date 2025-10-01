#include "core/TexturePanel.h"
#include "core/SpriteSheet.h"
#include "core/SpritePayload.h"
#include "core/AnimationMap.h"
#include "core/AnimationPlayer.h"
#include "core/AssetManager.h"
#include "core/Sprite.h"

#include "util/PathUtils.h"
#include "util/Time.h"

#include <functional>
#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <filesystem>
#include <fstream>
#include <memory>

namespace fs = std::filesystem;

inline std::vector<std::string> getTextureFiles(const std::string &folderPath)
{
    std::vector<std::string> textures;
    for (const auto &file : fs::directory_iterator(folderPath))
    {
        if (file.is_regular_file())
        {
            std::string ext = file.path().extension().string();
            if (ext == ".json")
            {
                textures.push_back(file.path().string());
            }
        }
    }

    // TODO: returning a copy everytime it gets called
    return textures;
}

TexturePanel::TexturePanel(EditorContext &ctx) : EditorPanel{ctx}, m_ctx{ctx}
{
}

TexturePanel::~TexturePanel()
{
}

void TexturePanel::draw()
{
    renderTextureAssetsListPanel();
    copyTextureToAssets();
    renderSelectedTexSheetPanel(false);
    renderAnimationPanel();
}

void TexturePanel::renderTextureAssetsListPanel()
{
    ImGui::Begin("Texture Resources");
    if (ImGui::Button("Add Texture"))
    {
        ImGuiFileDialog::Instance()->OpenDialog(
            "ChooseTexFile", "Select a Texture",
            ".png,.jpg,.jpeg");
    }
    // TODO: lets avoid this call on every render
    auto texturesJsonPath = getTextureFiles(
        getFilePath("assets/texture"));
    for (const auto &textureJsonPath : texturesJsonPath)
    {
        std::string fileName = fs::path(textureJsonPath).filename().string();

        if (ImGui::Selectable(fileName.c_str(), m_ctx.selectedTextureJsonPath == textureJsonPath))
        {
            m_ctx.selectedTextureJsonPath = textureJsonPath;
        }
    }

    ImGui::End();
}

// only copies assets if one is selected
void TexturePanel::copyTextureToAssets()
{
    if (ImGuiFileDialog::Instance()->Display("ChooseTexFile"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string selectedPath = ImGuiFileDialog::Instance()->GetFilePathName();
            std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
            std::string destPath = getFilePath("assets/texture") / fileName;

            try
            {
                std::filesystem::copy_file(
                    selectedPath, destPath,
                    std::filesystem::copy_options::update_existing);
            }
            catch (const std::filesystem::filesystem_error &e)
            {
                std::cerr << "Failed to copy texture: " << e.what() << std::endl;
            }
        }

        ImGuiFileDialog::Instance()->Close();
    }
}

void TexturePanel::renderAnimationPanel()
{
    ImGui::Begin("Animations");

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
    for(auto &[animName, animation]: animationMap->getAnimations()) {
        auto it = animationPlayerMap.find(animName);
        if (it == animationPlayerMap.end())
        {
            animationPlayerMap[animName] = AnimationPlayer{animation};
        }
        animationPlayerMap[animName].update();

        std::vector<glm::vec2> texCoord = animationPlayerMap[animName].getCurrentFrame().sprite.getTextureCoordinates();
        ImVec2 topLeft = ImVec2(texCoord[0].x, texCoord[0].y);
        ImVec2 bottomRight = ImVec2(texCoord[2].x, texCoord[2].y);

        float imgButtonWidth = 64;
        float imgButtonHeight = 64;

        ImGui::PushID(imguiId);
        if (ImGui::ImageButton(
                "",
                texId,
                ImVec2(imgButtonWidth, imgButtonHeight),
                topLeft,
                bottomRight,
                ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
                ImVec4(1.0f, 1.0f, 1.0f, 1.0f)))
        {
        }

        ImGui::PopID();
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

void TexturePanel::renderSelectedTexSheetPanel(bool isInModal, std::function<void(Sprite &sprite)> onClick)
{
    if (!isInModal)
    {
        ImGui::Begin("Sprites");
    }

    if (m_ctx.selectedTextureJsonPath.empty())
    {
        if (!isInModal)
        {
            ImGui::End();
        }
        return;
    }

    std::shared_ptr<SpriteSheet> spriteSheet = AssetManager::instance().getSpriteSheet(m_ctx.selectedTextureJsonPath);

    bool changed = false;

    ImGui::PushItemWidth(80);
    changed |= ImGui::DragFloat("W", &spriteSheet->m_spriteW);
    ImGui::SameLine();
    changed |= ImGui::DragFloat("H", &spriteSheet->m_spriteH);
    ImGui::SameLine();
    changed |= ImGui::DragFloat("GapX", &spriteSheet->m_spriteGapX);
    ImGui::SameLine();
    changed |= ImGui::DragFloat("GapY", &spriteSheet->m_spriteGapY);

    if (changed)
    {
        spriteSheet->updateSpriteSizes();
    }

    std::shared_ptr<Texture> spriteSheetTexture = spriteSheet->getTexture();
    int id = 0;
    for (Sprite sprite : spriteSheet->getSprites())
    {
        float imgButtonWidth = 64;
        float imgButtonHeight = 64;
        std::vector<glm::vec2> textureCoordinates = sprite.getTextureCoordinates();
        ImTextureID texId = (ImTextureID)(uintptr_t)spriteSheetTexture->getTextureId();

        // TODO: Add sprite IDs and use those to identify which sprite was clicked
        ImGui::PushID(id);
        if (ImGui::ImageButton(
                "",
                texId,
                ImVec2(imgButtonWidth, imgButtonHeight),
                ImVec2(textureCoordinates[0].x,
                       textureCoordinates[0].y), // uv0 = top-left
                ImVec2(textureCoordinates[2].x,
                       textureCoordinates[2].y), // uv1 = bottom-right
                ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
                ImVec4(1.0f, 1.0f, 1.0f, 1.0f)))
        {
            if (onClick != nullptr)
            {
                onClick(sprite);
            }
        }

        // TODO: drag and drop should only be enabled when not in modal form
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            SpritePayload payload{id};

            ImGui::SetDragDropPayload("SPRITE", &payload, sizeof(SpritePayload));
            ImGui::Text("Dragging sprite %d", id);
            ImGui::EndDragDropSource();
        }

        ImGui::PopID();
        id++;

        // handle wrapping
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();

        float windowX2 = windowPos.x + windowSize.x;
        ImVec2 lastSpritePosition = ImGui::GetItemRectMax();
        float lastSpriteX2 = lastSpritePosition.x;
        float nextButtonX2 = lastSpriteX2 + imgButtonWidth;
        if (id < spriteSheet->getSprites().size() && nextButtonX2 < windowX2)
        {
            ImGui::SameLine();
        }
    }

    if (!isInModal)
    {
        ImGui::End();
    }
}
