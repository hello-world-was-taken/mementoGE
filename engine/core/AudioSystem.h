#pragma once

#include "core/components/AudioSource.h"

#include "core/ISystem.h"

class AudioSystem : public ISystem
{
public:
    AudioSystem();
    ~AudioSystem() = default;

    void update(SystemContext &context) override;
    SystemDescriptor descriptor() const override;

private:
    static constexpr float WORLD_GAIN_FADE_PER_SECOND = 4.0f;

    float m_worldGain = 1.0f;

    void updateListener(float deltaTime);
};
