#include "core/components/Sprite.h"

#include "core/AnimationMap.h"
#include "core/AnimationPlayer.h"
#include "core/AssetManager.h"
#include "core/SpriteSheet.h"

#include "editor/DragNDropPayloads.h"
#include "editor/TexturePanel.h"

#include "util/PathUtils.h"
#include "util/Time.h"

#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <filesystem>
#include <fstream>
#include <functional>
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

    // TODO: returning a copy everytime it gets called. NRVO maybe its fine?
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
}

void TexturePanel::renderTextureAssetsListPanel()
{
    ImGui::Begin("Texture Resources");
    if (ImGui::Button("Add Texture"))
    {
        ImGuiFileDialog::Instance()->OpenDialog("ChooseTexFile", "Select a Texture", ".png,.jpg,.jpeg");
    }
    // TODO: lets avoid this call on every render
    auto texturesJsonPath = getTextureFiles(getGameAssetsPath("texture"));
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
            std::string destPath = getFilePath("texture") / fileName;

            try
            {
                std::filesystem::copy_file(selectedPath, destPath, std::filesystem::copy_options::update_existing);
            }
            catch (const std::filesystem::filesystem_error &e)
            {
                std::cerr << "Failed to copy texture: " << e.what() << std::endl;
            }
        }

        ImGuiFileDialog::Instance()->Close();
    }
}
