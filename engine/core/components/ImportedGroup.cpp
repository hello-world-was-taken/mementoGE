#include "core/components/ImportedGroup.h"

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"

#include <imgui.h>
#endif

void ImportedGroup::serialize(YAML::Emitter &out) const
{
    out << YAML::Key << "ImportedGroup" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "GroupId" << YAML::Value << groupId;
    out << YAML::Key << "SourceAsset" << YAML::Value << sourceAsset;
    out << YAML::EndMap;
}

void ImportedGroup::deserialize(const YAML::Node &in)
{
    const YAML::Node node = in["ImportedGroup"];
    groupId = node["GroupId"] ? node["GroupId"].as<uint64_t>() : 0;
    sourceAsset = node["SourceAsset"] ? node["SourceAsset"].as<std::string>() : std::string{};
}

#ifdef EDITOR_BUILD
void ImportedGroup::drawInspector()
{
    ImGuiWrapper::Collapsable("Imported Group",
        [&]
        {
            ImGui::Text("Group ID: %llu", static_cast<unsigned long long>(groupId));
            ImGui::TextWrapped("Source: %s", sourceAsset.c_str());
        });
}
#endif
