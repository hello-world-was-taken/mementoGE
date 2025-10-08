#include "core/SpriteSheet.h"
#include "core/AssetManager.h"
#include "core/Sprite.h"
#include "core/Animator.h"

#include "editor/PropertiesPanel.h"
#include "editor/EditorPanel.h"
#include "editor/TexturePanel.h"
#include "editor/EditorContext.h"

#include "physics/CircleCollider2D.h"

#include "util/PathUtils.h"

#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <memory>

namespace fs = std::filesystem;

inline void SetFieldWidth(float w = 120.0f)
{
    ImGui::SetNextItemWidth(w);
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
    renderPropertiesInWindow();
    renderPropertiesInPopup();
}

void PropertiesPanel::renderPropertiesInWindow()
{
    ImGui::Begin("Properties");
    renderPropertiesPanel();
    ImGui::End();
}

void PropertiesPanel::renderPropertiesInPopup()
{
    if (!m_ctx.showPropertiesPopup)
        return;

    // Place popup a little to the right of mouse click
    ImGui::SetNextWindowPos(m_ctx.propertiesPopupPos);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_Appearing);
    if (ImGui::BeginPopup("PropertiesPopup"))
    {
        ImGui::BeginChild("PropertiesScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding);

        renderPropertiesPanel();

        ImGui::EndChild();
        ImGui::EndPopup();
    }
    else
    {
        m_ctx.showPropertiesPopup = false;
    }
}

void PropertiesPanel::renderPropertiesPanel()
{
    if (m_ctx.sceneManager.getActiveScene().getGameObjects().empty())
        return;

    GameObject *go = m_ctx.sceneManager.getActiveScene().getActiveGameObject();
    if (!go)
    {
        ImGui::Text("No game object selected");
        return;
    }

    drawIdentity(go);
    drawSize(go);
    drawTransform(go);
    drawLayer(go);
    drawSpriteSettings(go);
    drawAddComponentCombo(go);
    drawRigidBodySettings(go);
    drawBoxColliderSettings(go);
    drawAnimatorSettings(go);

    // Popups (global)
    drawPopups();
}

void PropertiesPanel::drawIdentity(GameObject *go)
{
    ImGui::Separator();
    ImGui::Text("Identity");

    // Display Entity ID (non-editable)
    ImGui::Text("Entity ID: %u", (unsigned int)go->getEntityId());

    // TODO: there is a way we can directly support string.
    static char tagBuffer[128];
    std::string tag = go->getTag();
    strncpy(tagBuffer, tag.c_str(), sizeof(tagBuffer));
    tagBuffer[sizeof(tagBuffer) - 1] = '\0';

    SetFieldWidth(200);
    if (ImGui::InputText("Tag", tagBuffer, IM_ARRAYSIZE(tagBuffer)))
    {
        go->setTag(std::string(tagBuffer));
    }
}

void PropertiesPanel::drawSize(GameObject *go)
{
    ImGui::Text("Size");
    int width = go->getWidth();
    int height = go->getHeight();

    SetFieldWidth();
    if (ImGui::DragInt("Width", &width))
        go->setWidth(width);

    SetFieldWidth();
    if (ImGui::DragInt("Height", &height))
        go->setHeight(height);
}

void PropertiesPanel::drawTransform(GameObject *go)
{
    ImGui::Separator();
    ImGui::Text("Transform");
    Transform &transform = go->getComponent<Transform>();
    glm::vec3 *pos = transform.getPosition();

    SetFieldWidth();
    ImGui::DragFloat("x", &pos->x);

    SetFieldWidth();
    ImGui::DragFloat("y", &pos->y);
}

void PropertiesPanel::drawLayer(GameObject *go)
{
    ImGui::Separator();
    ImGui::Text("Layer");
    Transform &transform = go->getComponent<Transform>();
    glm::vec3 *pos = transform.getPosition();
    int layer = static_cast<int>(pos->z);

    SetFieldWidth();
    if (ImGui::DragInt("Layer", &layer))
        pos->z = static_cast<float>(layer);
}

void PropertiesPanel::drawSpriteSettings(GameObject *go)
{
    if (!go->hasComponent<Sprite>())
        return;

    Sprite &sprite = go->getComponent<Sprite>();

    ImGui::Separator();
    ImGui::Text("Sprite Settings");

    static bool flipX = sprite.isFlippedX();
    static bool flipY = sprite.isFlippedY();
    if (ImGui::Checkbox("Flip Horizontally", &flipX))
        sprite.setFlipX(flipX);
    if (ImGui::Checkbox("Flip Vertically", &flipY))
        sprite.setFlipY(flipY);

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

void PropertiesPanel::drawAddComponentCombo(GameObject *go)
{
    SetFieldWidth(150);
    if (ImGui::BeginCombo("Add Component", "Select..."))
    {
        if (ImGui::Selectable("Rigidbody2D"))
        {
            go->addComponent<RigidBody2D>();
            m_ctx.sceneManager.getActiveScene().getPhysics2d().addRigidbody(*go);
        }

        if (ImGui::Selectable("BoxCollider2D"))
        {
            go->addComponent<BoxCollider2D>(go->getWidth(), go->getHeight());
            m_ctx.sceneManager.getActiveScene().getPhysics2d().addRigidbody(*go);
        }

        if (ImGui::Selectable("CircleCollider2D"))
        {
            go->addComponent<CircleCollider2D>();
            m_ctx.sceneManager.getActiveScene().getPhysics2d().addRigidbody(*go);
        }

        if (ImGui::Selectable("Animator"))
        {
            if (!m_ctx.selectedTextureJsonPath.empty())
            {
                auto animMap = AssetManager::instance().getAnimationMap(m_ctx.selectedTextureJsonPath);
                go->addComponent<Animator>(animMap, "idle");
            }
            else
            {
                ImGui::OpenPopup("MissingAnimationPopup");
            }
        }
        ImGui::EndCombo();
    }
}

void PropertiesPanel::drawRigidBodySettings(GameObject *go)
{
    if (!go->hasComponent<RigidBody2D>())
        return;

    RigidBody2D &rb = go->getComponent<RigidBody2D>();
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

void PropertiesPanel::drawBoxColliderSettings(GameObject *go)
{
    if (!go->hasComponent<BoxCollider2D>())
        return;

    auto &box = go->getComponent<BoxCollider2D>();
    ImGui::Text("Box Collider 2D");

    SetFieldWidth();
    ImGui::DragFloat("Density", &box.m_density, 0.01f, 0.0f);

    SetFieldWidth();
    ImGui::DragFloat("Friction", &box.m_friction, 0.01f, 0.0f, 1.0f);

    SetFieldWidth();
    ImGui::DragFloat("Restitution", &box.m_restitution, 0.01f, 0.0f, 1.0f);
}

void PropertiesPanel::drawAnimatorSettings(GameObject *go)
{
    if (!go->hasComponent<Animator>())
        return;

    auto &animator = go->getComponent<Animator>();

    ImGui::Separator();
    ImGui::Text("Animator");

    ImGui::Text("Current Animation: %s", animator.currentAnimation.c_str());

    SetFieldWidth(150);
    if (ImGui::BeginCombo("Animation", animator.currentAnimation.c_str()))
    {
        for (auto &[name, anim] : animator.animationMap->getAnimations())
        {
            bool selected = (name == animator.currentAnimation);
            if (ImGui::Selectable(name.c_str(), selected))
            {
                animator.play(name, anim.loop);
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button(animator.animationPlayer.isPlaying() ? "Pause" : "Play"))
    {
        if (animator.animationPlayer.isPlaying())
            animator.animationPlayer.pause();
        else
            animator.animationPlayer.play(animator.animationMap->getAnimation(animator.currentAnimation), true);
    }

    ImGui::SameLine();
    if (ImGui::Button("Stop"))
        animator.animationPlayer.stop();
}

void PropertiesPanel::drawPopups()
{
    // Missing texture
    if (ImGui::BeginPopupModal("MissingTexturePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Please select a texture first!");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // Sprite picker
    if (ImGui::BeginPopupModal("Select Sprite", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        m_texturePanel.renderSelectedTexSheetPanel(true, [&](Sprite &sprite)
                                                   {
            if (auto go = m_ctx.sceneManager.getActiveScene().getActiveGameObject()) {
                if (go->hasComponent<Sprite>())
                    go->removeComponent<Sprite>();

                go->addComponent<Sprite>(
                    getTexturePathFromJson(m_ctx.selectedTextureJsonPath),
                    sprite.getTextureCoordinates());
            } });
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // Missing animation
    if (ImGui::BeginPopupModal("MissingAnimationPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Please select an animation JSON first!");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}
