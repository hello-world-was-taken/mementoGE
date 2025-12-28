#include "core/AudioSystem.h"

#include "core/components/AudioSource.h"

#include <AL/al.h>
#include <glm/glm.hpp>

AudioSystem::AudioSystem()
{
    m_device = UniqueDevicePtr(alcOpenDevice(nullptr));
    if (!m_device)
    {
        throw std::runtime_error("Failed to open OpenAL device");
    }

    m_context = UniqueContextPtr(alcCreateContext(m_device.get(), nullptr));
    if (!m_context)
    {
        throw std::runtime_error("Failed to create OpenAL context");
    }

    alcMakeContextCurrent(m_context.get());
}

void AudioSystem::update(entt::registry &registry)
{
    // Update listener (camera)
    updateListener(registry);

    auto view = registry.view<AudioSource>();

    for (auto entity : view)
    {
        AudioSource &audioSource = view.get<AudioSource>(entity);

        // Keep gameobject's and audioplayer's position in sync
        if (registry.any_of<Transform>(entity))
        {
            const auto &transform = registry.get<Transform>(entity);
            audioSource.audioPlayer.setPosition(transform.position);
        }

        audioSource.audioPlayer.play(audioSource.filePath);
    }
}

void AudioSystem::updateListener(entt::registry &registry)
{
    glm::vec3 pos{0.0f};
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};

    // TODO: query camera component here

    alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
    // alListener3f(AL_VELOCITY, vel.x, vel.y, vel.z);

    float orientation[6] = {forward.x, forward.y, forward.z, up.x, up.y, up.z};

    alListenerfv(AL_ORIENTATION, orientation);
}
