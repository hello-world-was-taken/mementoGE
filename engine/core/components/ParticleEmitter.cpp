#include "core/components/ParticleEmitter.h"

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"

#include <imgui.h>

void ParticleEmitter::serialize(YAML::Emitter &out)
{

    out << YAML::Key << "ParticleEmitter";
    out << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "rate" << YAML::Value << rate;
    out << YAML::Key << "minLife" << YAML::Value << minLife;
    out << YAML::Key << "maxLife" << YAML::Value << maxLife;
    out << YAML::Key << "minSize" << YAML::Value << minSize;
    out << YAML::Key << "maxSize" << YAML::Value << maxSize;

    out << YAML::Key << "spawnOffset" << YAML::Value << YAML::Flow << YAML::BeginSeq << spawnOffset.x << spawnOffset.y
        << spawnOffset.z << YAML::EndSeq;

    out << YAML::Key << "baseVelocity" << YAML::Value << YAML::Flow << YAML::BeginSeq << baseVelocity.x
        << baseVelocity.y << baseVelocity.z << YAML::EndSeq;
    out << YAML::Key << "baseColor" << YAML::Value << YAML::Flow << YAML::BeginSeq << baseColor.r << baseColor.g
        << baseColor.b << baseColor.a << YAML::EndSeq;

    out << YAML::EndMap;
}

void ParticleEmitter::deserialize(const YAML::Node &node)
{
    if (!node["ParticleEmitter"])
    {
        return;
    }

    const auto &em = node["ParticleEmitter"];

    if (em["rate"])
    {
        rate = em["rate"].as<float>();
    }
    if (em["minLife"])
    {
        minLife = em["minLife"].as<float>();
    }
    if (em["maxLife"])
    {
        maxLife = em["maxLife"].as<float>();
    }
    if (em["minSize"])
    {
        minSize = em["minSize"].as<float>();
    }
    if (em["maxSize"])
    {
        maxSize = em["maxSize"].as<float>();
    }

    if (em["spawnOffset"])
    {
        auto v = em["spawnOffset"];
        spawnOffset = {v[0].as<float>(), v[1].as<float>(), v[2].as<float>()};
    }

    if (em["baseVelocity"])
    {
        auto v = em["baseVelocity"];
        baseVelocity = {v[0].as<float>(), v[1].as<float>(), v[2].as<float>()};
    }

    if (em["baseColor"])
    {
        auto c = em["baseColor"];
        baseColor = {c[0].as<float>(), c[1].as<float>(), c[2].as<float>(), c[3].as<float>()};
    }
}

void ParticleEmitter::drawInspector()
{
    ImGuiWrapper::Collapsable("Particle Emitter",
        [&]()
        {
            ImGui::DragFloat("Rate (pps)", &rate, 1.0f, 0.0f, 1000.0f);

            ImGui::DragFloat("Min Life", &minLife, 0.01f, 0.01f, 10.0f);
            ImGui::DragFloat("Max Life", &maxLife, 0.01f, 0.01f, 10.0f);

            if (minLife > maxLife)
            {
                minLife = maxLife;
            }

            ImGui::DragFloat("Min Size", &minSize, 0.01f, 0.01f, 10.0f);
            ImGui::DragFloat("Max Size", &maxSize, 0.01f, 0.01f, 10.0f);

            if (minSize > maxSize)
            {
                minSize = maxSize;
            }

            ImGui::DragFloat3("Spawn Offset", &spawnOffset[0], 0.01f);
            ImGui::DragFloat3("Base Velocity", &baseVelocity[0], 0.01f);
            ImGui::ColorEdit4("Base Color", &baseColor[0]);
        });
}

#endif
