#include "core/PropertiesPanel.h"
#include "core/EditorPanel.h"
#include "core/TexturePanel.h"
#include "core/EditorContext.h"
#include "core/SpriteSheet.h"
#include "core/AssetManager.h"
#include "core/Sprite.h"

#include "physics/CircleCollider2D.h"

#include "util/PathUtils.h"

#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <memory>

namespace fs = std::filesystem;

// TODO: move this to a dedicated utilities file
// for now keep insync with TexturePanel.getTextureFiles
// remove fstream / filesystem if they are not needed after removal
inline std::vector<std::string> getTextureFiles(const std::string &folderPath)
{
    std::vector<std::string> textures;
    for (const auto &file : fs::directory_iterator(folderPath))
    {
        if (file.is_regular_file())
        {
            std::string ext = file.path().extension().string();
            // TODO: on-click for the list, return the texture within the json
            if (ext == ".json")
            {
                textures.push_back(file.path().string());
            }
        }
    }

    // TODO: returning a copy everytime it gets called
    return textures;
}

PropertiesPanel::PropertiesPanel(EditorContext &ctx, TexturePanel &texturePanel)
    : EditorPanel{ctx},
      m_ctx{ctx},
      m_texturePanel{texturePanel}
{
}

PropertiesPanel::~PropertiesPanel()
{
}

void PropertiesPanel::draw()
{
    renderPropertiesPanel();
}

void PropertiesPanel::renderPropertiesPanel()
{
    if (m_ctx.sceneManager.getActiveScene().getGameObjects().empty())
        return;

    ImGui::Begin("Properties");

    GameObject *go = m_ctx.sceneManager.getActiveScene().getActiveGameObject();
    if (!go)
    {
        ImGui::Text("No game object selected");
        ImGui::End();
        return;
    }

    // Size
    ImGui::Text("Size");
    int width = go->getWidth();
    int height = go->getHeight();
    if (ImGui::DragInt("Width", &width))
        go->setWidth(width);
    if (ImGui::DragInt("Height", &height))
        go->setHeight(height);

    // Transform
    ImGui::Separator();
    ImGui::Text("Transform");
    Transform &transform = go->getComponent<Transform>();
    glm::vec3 *pos = transform.getPosition();
    ImGui::DragFloat("x", &pos->x);
    ImGui::DragFloat("y", &pos->y);
    // ImGui::DragFloat("z", &pos->z);

    // Layer
    ImGui::Separator();
    ImGui::Text("Layer");
    int layer = static_cast<int>(pos->z);
    if (ImGui::DragInt("Layer", &layer))
        pos->z = static_cast<float>(layer);

    // Sprite Settings
    if (go->hasComponent<Sprite>())
    {
        Sprite &sprite = go->getComponent<Sprite>();

        ImGui::Separator();
        ImGui::Text("Sprite Settings");

        // Flip
        static bool flipX = sprite.isFlippedX();
        static bool flipY = sprite.isFlippedY();
        if (ImGui::Checkbox("Flip Horizontally", &flipX))
            sprite.setFlipX(flipX);
        if (ImGui::Checkbox("Flip Vertically", &flipY))
            sprite.setFlipY(flipY);

        // Open sprite picker modal
        if (ImGui::Button("Change Sprite"))
        {
            if (m_ctx.selectedTextureJsonPath.empty())
            {
                ImGui::OpenPopup("MissingTexturePopup");
            }
            else
            {
                ImGui::OpenPopup("Select Sprite");
            }
        }

        std::string texPath = sprite.getTexturePath();
        ImGui::Text("Current Sprite:");
        ImGui::TextWrapped("%s", texPath.c_str());
    }

    if (ImGui::BeginPopupModal("MissingTexturePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Please select a texture first!");
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Select Sprite", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        m_texturePanel.renderSelectedTexSheetPanel(
            true,
            [&](Sprite &sprite)
            {
            if (m_ctx.sceneManager.getActiveScene().getActiveGameObject())
            {
                GameObject *go = m_ctx.sceneManager.getActiveScene().getActiveGameObject();
                // first remove Sprite if it exists
                if (go->hasComponent<Sprite>())
                    go->removeComponent<Sprite>();

                m_ctx.sceneManager.getActiveScene().getActiveGameObject()->addComponent<Sprite>(
                    getTexturePathFromJson(m_ctx.selectedTextureJsonPath),
                    sprite.getTextureCoordinates());
                // ImGui::CloseCurrentPopup(); // Close modal
            } });

        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    if (ImGui::BeginCombo("Add Component", "Select..."))
    {
        if (ImGui::Selectable("Rigidbody2D"))
        {
            go->addComponent<Rigidbody2D>();
            m_ctx.sceneManager.getActiveScene().getPhysics2d().addRigidbody(*go);
        }

        if (ImGui::Selectable("BoxCollider2D"))
        {
            int width = go->getWidth();
            int height = go->getHeight();
            go->addComponent<BoxCollider2D>(width, height);
            m_ctx.sceneManager.getActiveScene().getPhysics2d().addRigidbody(*go);
        }

        if (ImGui::Selectable("CircleCollider2D"))
        {
            go->addComponent<CircleCollider2D>();
            m_ctx.sceneManager.getActiveScene().getPhysics2d().addRigidbody(*go);
        }

        ImGui::EndCombo();
    }

    // If the active game object have rigid body 2d, we should be able to
    // edit whether it should be static, dynamic or kinematics
    if (go->hasComponent<Rigidbody2D>())
    {
        Rigidbody2D &rb = go->getComponent<Rigidbody2D>();
        if (ImGui::BeginCombo("Rigidbody 2D Type", rb.getBodyType().c_str()))
        {
            if (ImGui::Selectable("Static"))
                rb.setType(BodyType::Static);

            if (ImGui::Selectable("Dynamic"))
                rb.setType(BodyType::Dynamic);

            if (ImGui::Selectable("Kinematic"))
                rb.setType(BodyType::Kinematic);

            ImGui::EndCombo();
        }
    }

    if (go->hasComponent<BoxCollider2D>())
    {
        auto &box = go->getComponent<BoxCollider2D>();
        ImGui::Text("Box Collider 2D");
        // ImGui::DragFloat2("Size", glm::value_ptr(box.m_size), 0.1f);
        // ImGui::DragFloat2("Offset", glm::value_ptr(box.m_offset), 0.1f);
        ImGui::DragFloat("Density", &box.m_density, 0.01f, 0.0f);
        ImGui::DragFloat("Friction", &box.m_friction, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Restitution", &box.m_restitution, 0.01f, 0.0f, 1.0f);
    }

    ImGui::End();
}
