#include "core/components/CircleCollider2D.h"
#include "core/components/EnemyState.h"
#include "core/components/Patrol.h"
#include "core/components/Sprite.h"
#include "core/components/Transform.h"

#include "core/Animator.h"
#include "core/AssetManager.h"
#include "core/SpriteSheet.h"

#include "editor/EditorContext.h"
#include "editor/EditorPanel.h"
#include "editor/PropertiesPanel.h"
#include "editor/SpritePayload.h"
#include "editor/TexturePanel.h"

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
    : EditorPanel{ctx}, m_ctx{ctx}, m_texturePanel{texturePanel}
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

    // TODO: look into entt::meta
    drawIdentity(go);
    drawSize(go);
    drawComponentInspector<Transform>(*go);
    drawComponentInspector<BoxCollider2D>(*go);
    drawComponentInspector<RigidBody2D>(*go);
    drawComponentInspector<EnemyState>(*go);
    drawComponentInspector<Patrol>(*go);
    drawSpriteSettings(go);
    drawAddComponentCombo(go);
    drawAnimatorSettings(go);

    ImGui::Separator();
    if (ImGui::Button("Delete Object"))
    {
        m_ctx.sceneManager.getActiveScene().removeGameObject(go->getEntityId());
    }

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
    ImGui::Separator();
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

void PropertiesPanel::drawSpriteSettings(GameObject *go)
{
    if (!go->hasComponent<Sprite>())
        return;

    Sprite &sprite = go->getComponent<Sprite>();

    ImGui::Separator();
    ImGui::Text("Sprite Settings");

    if (ImGui::Checkbox("Flip Horizontally", &sprite.flipX))
        if (ImGui::Checkbox("Flip Vertically", &sprite.flipY))

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

    std::string texPath = sprite.texture->getFilePath();
    ImGui::Text("Current Sprite:");
    ImGui::TextWrapped("%s", texPath.c_str());
}

void PropertiesPanel::drawAddComponentCombo(GameObject *go)
{
    ImGui::Separator();
    SetFieldWidth(150);
    // TODO: make them selectable at the component level like the other editor extensions
    if (ImGui::BeginCombo("Add Component", "Select..."))
    {
        if (ImGui::Selectable("Rigidbody2D"))
        {
            go->addComponent<RigidBody2D>();
            m_ctx.sceneManager.getActiveScene().getPhysics2d().addRigidbody(*go);
        }

        if (ImGui::Selectable("BoxCollider2D"))
        {
            go->addComponent<BoxCollider2D>();
            go->getComponent<BoxCollider2D>().size = {go->getWidth(), go->getHeight()};
            m_ctx.sceneManager.getActiveScene().getPhysics2d().addRigidbody(*go);
        }

        if (ImGui::Selectable("CircleCollider2D"))
        {
            go->addComponent<CircleCollider2D>();
            m_ctx.sceneManager.getActiveScene().getPhysics2d().addRigidbody(*go);
        }

        if (ImGui::Selectable("Animator"))
        {
            go->addComponent<Animator>();
        }

        if (ImGui::Selectable("Enemy Stats"))
        {
            go->addComponent<EnemyState>();
        }

        if (ImGui::Selectable("Patrol"))
        {
            go->addComponent<Patrol>();
        }
        ImGui::EndCombo();
    }
}

void PropertiesPanel::drawRigidBodySettings(GameObject *go)
{
    if (!go->hasComponent<RigidBody2D>())
        return;

    RigidBody2D &rb = go->getComponent<RigidBody2D>();
    rb.drawInspector();
}

void PropertiesPanel::drawAnimatorSettings(GameObject *go)
{
    if (go->hasComponent<EnemyState>())
    {
        ImGui::Separator();
        ImGui::Text("has enemyAiState");
    }
    if (!go->hasComponent<Animator>())
        return;

    auto &animator = go->getComponent<Animator>();

    ImGui::Separator();
    ImGui::BeginChild("AnimatorBox", ImVec2(0, 150), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("Animator");

    ImGui::Text("Current Animation: %s", animator.currentAnimation.c_str());

    SetFieldWidth(150);
    if (ImGui::BeginCombo("Animation", animator.currentAnimation.c_str()))
    {
        for (auto &[name, animJsonPath] : animator.animationSourceMap)
        {
            bool selected = (name == animator.currentAnimation);
            if (ImGui::Selectable(name.c_str(), selected))
            {
                // TODO: do we need the loop?
                animator.play(name, true);
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
        {
            if (!animator.currentAnimation.empty())
            {
                animator.play(animator.currentAnimation); // TODO: why pass it, just use it inside
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        animator.animationPlayer.stop();
    }

    ImGui::EndChild();
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ANIMATION"))
        {
            IM_ASSERT(payload->DataSize == sizeof(AnimationPayload));
            AnimationPayload animPayload = *((AnimationPayload *)payload->Data);

            auto *go = m_ctx.sceneManager.getActiveScene().getActiveGameObject();
            go->getComponent<Animator>().animationSourceMap[animPayload.animationName] = animPayload.animationJsonPath;
        }
        ImGui::EndDragDropTarget();
    }
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
        m_texturePanel.renderSelectedTexSheetPanel(true,
            [&](Sprite &sprite)
            {
                if (auto go = m_ctx.sceneManager.getActiveScene().getActiveGameObject())
                {
                    if (go->hasComponent<Sprite>())
                        go->removeComponent<Sprite>();

                    go->addComponent<Sprite>(sprite.topLeft, sprite.width, sprite.height, sprite.texture);
                }
            });
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}
