#include "core/components/AudioSource.h"

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"
#include "editor/DragNDropPayloads.h"
#include <imgui.h>
#endif

#ifdef EDITOR_BUILD
void AudioSource::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "AudioSource";
    out << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "FilePath" << YAML::Value << filePath;
    out << YAML::Key << "Looping" << YAML::Value << looping;
    out << YAML::Key << "Gain" << YAML::Value << gain;
    out << YAML::Key << "Pitch" << YAML::Value << pitch;

    out << YAML::EndMap;
}

void AudioSource::deserialize(const YAML::Node &in)
{
    auto node = in["AudioSource"];
    filePath = node["FilePath"] ? node["FilePath"].as<std::string>() : "";
    looping = node["Looping"] ? node["Looping"].as<bool>() : false;
    gain = node["Gain"] ? node["Gain"].as<float>() : 1.0f;
    pitch = node["Pitch"] ? node["Pitch"].as<float>() : 1.0f;
}

void AudioSource::drawInspector()
{
    ImGuiWrapper::Collapsable("Audio Source",
        [&]
        {
            ImGui::Text("Audio: %s", filePath.empty() ? "None" : filePath.c_str());
            ImGui::SliderFloat("Gain", &gain, 0.0f, 2.0f);
            ImGui::SliderFloat("Pitch", &pitch, 0.5f, 2.0f);
            ImGui::Checkbox("Looping", &looping);

            handleAudioDrop();

            if (ImGui::Button("Play"))
            {
                audioPlayer.play(filePath);
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop"))
            {
                audioPlayer.stop();
            }
        });
}

void AudioSource::handleAudioDrop()
{
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("AUDIO"))
        {
            AudioPayload audioPayload = *(AudioPayload *)payload->Data;
            filePath = audioPayload.filePath;
        }
        ImGui::EndDragDropTarget();
    }
}
#endif
