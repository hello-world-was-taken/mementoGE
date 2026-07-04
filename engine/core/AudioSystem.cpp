#include "core/AudioSystem.h"
#include "core/AudioSettings.h"
#include "core/components/Transform.h"

#include "core/components/AudioSource.h"
#include "util/PathUtils.h"

#include <AL/al.h>
#include <algorithm>
#include <filesystem>
#include <glm/glm.hpp>
#include <vector>

SystemDescriptor AudioSystem::descriptor() const
{
    return SystemDescriptor::named("AudioSystem")
        .reads<Transform>()
        .writes<AudioSource>()
        .readsResource(SystemResource::AudioSettings)
        .writesResource(SystemResource::AudioDevice)
        // Destroying an audio entity may also trigger rigid-body cleanup.
        .writesResource(SystemResource::PhysicsWorld)
        .writesResource(SystemResource::AssetCache)
        .requestsStructuralChanges()
        .withAffinity(SystemThreadAffinity::AnyWorker)
        .after("FollowSystem")
        .before("PhysicsSystem")
        .nonReentrant();
}

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

void AudioSystem::update(SystemContext &context)
{
    SystemEntities &registry = context.entities;
    // Update listener (camera)
    updateListener(context.frame.deltaTime);

    auto view = registry.view<AudioSource>();
    std::vector<entt::entity> completedOneShots;

    for (auto entity : view)
    {
        AudioSource &audioSource = view.get<AudioSource>(entity);

        // Keep gameobject's and audioplayer's position in sync
        if (registry.any_of<Transform>(entity))
        {
            const auto &transform = registry.get<const Transform>(entity);
            audioSource.audioPlayer.setPosition(audioSource.relative ? glm::vec3{0.0f} : transform.position);
        }

        if (!audioSource.started && !audioSource.filePath.empty())
        {
            audioSource.audioPlayer.setLooping(audioSource.looping);
            audioSource.audioPlayer.setGain(audioSource.gain);
            audioSource.audioPlayer.setPitch(audioSource.pitch);
            audioSource.audioPlayer.setRelative(audioSource.relative);
            const std::filesystem::path configuredPath{audioSource.filePath};
            const std::string resolvedPath = configuredPath.is_absolute()
                                                 ? configuredPath.string()
                                                 : getGameAssetPath(audioSource.filePath).string();
            audioSource.audioPlayer.play(resolvedPath);
            audioSource.started = true;
        }
        else if (audioSource.started && audioSource.destroyWhenFinished && !audioSource.audioPlayer.isPlaying())
        {
            completedOneShots.push_back(entity);
        }
    }

    for (entt::entity entity : completedOneShots)
    {
        if (registry.valid(entity))
        {
            registry.destroy(entity);
        }
    }
}

void AudioSystem::updateListener(float deltaTime)
{
    glm::vec3 pos{0.0f};
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};

    // TODO: query camera component here

    const float targetGain = AudioSettings::worldGain();
    if (m_worldGain < targetGain)
    {
        m_worldGain = std::min(targetGain, m_worldGain + WORLD_GAIN_FADE_PER_SECOND * deltaTime);
    }
    else
    {
        m_worldGain = std::max(targetGain, m_worldGain - WORLD_GAIN_FADE_PER_SECOND * deltaTime);
    }

    alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
    alListenerf(AL_GAIN, AudioSettings::masterVolume() * m_worldGain);
    // alListener3f(AL_VELOCITY, vel.x, vel.y, vel.z);

    float orientation[6] = {forward.x, forward.y, forward.z, up.x, up.y, up.z};

    alListenerfv(AL_ORIENTATION, orientation);
}
