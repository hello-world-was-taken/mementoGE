#include "core/components/Camera.h"

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"
#include <imgui.h>
#endif

#ifdef EDITOR_BUILD

static const char *projectionTypeToString(CameraProjectionType t)
{
    switch (t)
    {
    case CameraProjectionType::Orthographic:
        return "Orthographic";
    }
    return "Orthographic";
}

static CameraProjectionType projectionTypeFromString(const std::string &s)
{
    if (s == "Orthographic")
    {
        return CameraProjectionType::Orthographic;
    }
    return CameraProjectionType::Orthographic;
}

void Camera::serialize(YAML::Emitter &out) const
{
    out << YAML::Key << "Camera";
    out << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "Primary" << YAML::Value << primary;

    out << YAML::Key << "ProjectionType" << YAML::Value << projectionTypeToString(projectionType);
    out << YAML::Key << "Zoom" << YAML::Value << zoom;
    out << YAML::Key << "MinZoom" << YAML::Value << minZoom;
    out << YAML::Key << "MaxZoom" << YAML::Value << maxZoom;

    out << YAML::Key << "LogicalWidth" << YAML::Value << logicalWidth;
    out << YAML::Key << "LogicalHeight" << YAML::Value << logicalHeight;

    out << YAML::Key << "NearClip" << YAML::Value << nearClip;
    out << YAML::Key << "FarClip" << YAML::Value << farClip;

    out << YAML::EndMap;
}

void Camera::deserialize(const YAML::Node &in)
{
    auto n = in["Camera"];
    if (!n)
    {
        return;
    }

    primary = n["Primary"].as<bool>();

    projectionType = projectionTypeFromString(n["ProjectionType"].as<std::string>());
    zoom = n["Zoom"].as<float>();
    minZoom = n["MinZoom"].as<float>();
    maxZoom = n["MaxZoom"].as<float>();

    logicalWidth = n["LogicalWidth"].as<float>();
    logicalHeight = n["LogicalHeight"].as<float>();

    nearClip = n["NearClip"].as<float>();
    farClip = n["FarClip"].as<float>();
}

void Camera::drawInspector()
{
    ImGuiWrapper::Collapsable("Camera",
        [&]
        {
            ImGui::Checkbox("Primary Camera", &primary);

            // Projection type (for future extensibility)
            const char *projNames[] = {"Orthographic"};
            int currentProj = 0;
            if (ImGui::Combo("Projection", &currentProj, projNames, IM_ARRAYSIZE(projNames)))
            {
                projectionType = CameraProjectionType::Orthographic;
            }

            ImGui::DragFloat("Zoom", &zoom, 0.01f, minZoom, maxZoom);
            ImGui::DragFloat("Min Zoom", &minZoom, 0.01f, 0.01f, 100.0f);
            ImGui::DragFloat("Max Zoom", &maxZoom, 0.01f, 0.01f, 100.0f);

            ImGui::Separator();

            ImGui::DragFloat("Logical Width", &logicalWidth, 1.0f, 1.0f, 5000.0f);
            ImGui::DragFloat("Logical Height", &logicalHeight, 1.0f, 1.0f, 5000.0f);

            ImGui::DragFloat("Near Clip", &nearClip, 1.0f, -10000.0f, 0.0f);
            ImGui::DragFloat("Far Clip", &farClip, 1.0f, 0.0f, 10000.0f);
        });
}

#endif
