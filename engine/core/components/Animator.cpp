#include "core/components/Animator.h"
#include "editor/DragNDropPayloads.h" // FIXME: editor shouldn't be a dependency in core

#include "core/AssetManager.h"

#include "util/PathUtils.h"

#include <iostream>
#include <memory>
#include <string>

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"

#include <imgui.h>
#include <yaml-cpp/yaml.h>
#endif

void Animator::play(std::string name, bool loop)
{
    auto it = animationSourceMap.find(name);
    if (it == animationSourceMap.end())
    {
        std::cout << "Animation " << name << " not found" << std::endl;
        name = "android";
        animationSourceMap[name] = getGameAssetsPath("texture/android.json");
    }

    std::string jsonPath = animationSourceMap[name];
    auto animMap = AssetManager::instance().getAnimationMap(jsonPath);

    currentAnimation = name;
    animationPlayer.play(animMap->getAnimation(name), loop);
}

void Animator::update()
{
    animationPlayer.update();
}

const Sprite &Animator::getCurrentSprite() const
{
    return animationPlayer.getCurrentFrame().sprite;
}

#ifdef EDITOR_BUILD
// FIXME: there are several places where SetFieldWidth is defined. Refactor
inline void SetFieldWidth(float w = 120.0f)
{
    ImGui::SetNextItemWidth(w);
}

void Animator::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "Animator";
    out << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "CurrentAnimation" << YAML::Value << currentAnimation;

    out << YAML::Key << "Animations";
    out << YAML::Value << YAML::BeginMap;
    for (auto &[animationName, animationPath] : animationSourceMap)
    {
        out << YAML::Key << animationName << YAML::Value << animationPath;
    }
    out << YAML::EndMap;

    out << YAML::EndMap;
}

void Animator::deserialize(const YAML::Node &node)
{
    if (!node["Animator"])
    {
        return;
    }

    auto animationsMap = node["Animator"]["Animations"];
    for (YAML::const_iterator it = animationsMap.begin(); it != animationsMap.end(); it++)
    {
        auto animationName = it->first.as<std::string>();
        auto animationJsonPath = it->second.as<std::string>();

        animationSourceMap[animationName] = animationJsonPath;
    }

    currentAnimation = node["Animator"]["CurrentAnimation"].as<std::string>();
    if (!currentAnimation.empty())
    {
        play(currentAnimation, true);
    }
}

void Animator::drawInspector()
{
    ImGui::Separator();
    ImGuiWrapper::Collapsable("Animator",
        [&]()
        {
            // TODO: do we need begin child for drag-n-drop to work here?
            // tried removing it and it wasn't working as expected
            ImGui::BeginChild("AnimatorBox", ImVec2(0, 150), true, ImGuiWindowFlags_NoScrollbar);
            ImGui::Text("Animator");

            ImGui::Text("Current Animation: %s", currentAnimation.c_str());

            SetFieldWidth(150);
            if (ImGui::BeginCombo("Animation", currentAnimation.c_str()))
            {
                for (auto &[name, animJsonPath] : animationSourceMap)
                {
                    bool selected = (name == currentAnimation);
                    if (ImGui::Selectable(name.c_str(), selected))
                    {
                        // TODO: do we need the loop?
                        play(name, true);
                    }
                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            if (ImGui::Button(animationPlayer.isPlaying() ? "Pause" : "Play"))
            {
                if (animationPlayer.isPlaying())
                {
                    animationPlayer.pause();
                }
                else
                {
                    if (!currentAnimation.empty())
                    {
                        play(currentAnimation); // TODO: why pass it, just use it inside
                    }
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Stop"))
            {
                animationPlayer.stop();
            }

            ImGui::EndChild();
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ANIMATION"))
                {
                    IM_ASSERT(payload->DataSize == sizeof(AnimationPayload));
                    AnimationPayload animPayload = *((AnimationPayload *)payload->Data);

                    animationSourceMap[animPayload.animationName] = animPayload.animationJsonPath;
                }
                ImGui::EndDragDropTarget();
            }
        });
}
#endif