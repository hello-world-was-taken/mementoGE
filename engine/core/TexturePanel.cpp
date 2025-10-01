#include "core/TexturePanel.h"
#include "core/SpriteSheet.h"
#include "core/SpritePayload.h"
#include "core/AnimationMap.h"
#include "core/Sprite.h"

#include "util/GetExecutableDir.h"
#include "util/Time.h"

#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <filesystem>
#include <fstream>
#include <memory>

namespace fs = std::filesystem;

// TODO: move this to a dedicated utilities file
// for now keep insync with PropertiesPanel.getTextureFiles
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

void TexturePanel::renderSelectedTexSheetPanel(bool isInModal)
{
    static int currentFrame = 0;
    static float currentFrameTime = 0.0f;

    if (!isInModal)
    {
        ImGui::Begin("Sprites");
    }

    // TODO: testing animation - remove
    auto path = getFilePath("assets/texture/run.json");
    auto animationMap = AnimationMap::fromJson(path);

    const Animation &burn = animationMap->getAnimation("burn");

    std::vector<glm::vec2> texCoord = burn.frames[currentFrame].sprite.getTextureCoordinates();

    ImVec2 topLeft = ImVec2(texCoord[0].x, texCoord[0].y);
    ImVec2 bottomRight = ImVec2(texCoord[2].x, texCoord[2].y);

    ImTextureID texIdNew = (ImTextureID)(uintptr_t)animationMap->getTexture()->getTextureId();

    ImGui::PushID(1000);
    if (ImGui::ImageButton(
            "",
            texIdNew,
            ImVec2(128, 128),
            topLeft,
            bottomRight,
            ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f)))
    {
        // No action here. Drag-n-drop using imgui
    }
    ImGui::PopID();

    if (currentFrameTime >= burn.frames[currentFrame].duration)
    {
        currentFrame++;
        currentFrameTime = 0.0f;
    }
    else
    {
        currentFrameTime += Time::deltaTime();
    }

    if (currentFrame == burn.frames.size())
    {
        currentFrame = 0;
        currentFrameTime = 0.0f;
    }

    if (m_ctx.selectedTextureJsonPath == "")
    {
        if (!isInModal)
        {
            ImGui::End();
        }
        return;
    }

    // TODO: we shouldn't be re-creating sprite sheet for an atlas, if we have already created it
    // somewhere else
    SpriteSheet spriteSheet = SpriteSheet::fromJson(m_ctx.selectedTextureJsonPath);
    std::shared_ptr<Texture> spriteSheetTexture = spriteSheet.getTexture();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();

    float windowX2 = windowPos.x + windowSize.x;
    int id = 0;
    for (Sprite sprite : spriteSheet.getSprites())
    {
        float imgButtonWidth = 32;
        float imgButtonHeight = 32;
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
            // No action here. Drag-n-drop using imgui
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            SpritePayload payload{id};

            ImGui::SetDragDropPayload("SPRITE", &payload, sizeof(SpritePayload));
            ImGui::Text("Dragging sprite %d", id);
            ImGui::EndDragDropSource();
        }

        ImGui::PopID();

        ImVec2 lastSpritePosition = ImGui::GetItemRectMax();
        float lastSpriteX2 = lastSpritePosition.x;
        float nextButtonX2 = lastSpriteX2 + imgButtonWidth;
        if (id + 1 < spriteSheet.getSprites().size() && nextButtonX2 < windowX2)
        {
            ImGui::SameLine();
        }
        id++;
    }

    if (!isInModal)
    {
        ImGui::End();
    }
}
