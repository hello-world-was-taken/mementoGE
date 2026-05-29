#include "core/AudioPlayer.h"
#include "core/AssetManager.h"

#include "util/LogChannels.h"
#include "util/log_error.h"

#include <sawyer/Logger.h>

AudioPlayer::AudioPlayer()
{
    alGenSources(1, &m_source);
    checkAlError("alGenSources");
}

AudioPlayer::~AudioPlayer()
{
    if (m_source != 0)
    {
        alDeleteSources(1, &m_source);
    }
}

AudioPlayer::AudioPlayer(AudioPlayer &&other) noexcept : m_source(other.m_source)
{
    other.m_source = 0;
}

AudioPlayer &AudioPlayer::operator=(AudioPlayer &&other) noexcept
{
    if (this != &other)
    {
        if (m_source != 0)
        {
            alDeleteSources(1, &m_source);
        }
        m_source = other.m_source;
        other.m_source = 0;
    }
    return *this;
}

// void AudioPlayer::setBuffer(ALuint buffer)
// {
//     alSourcei(m_source, AL_BUFFER, buffer);
// }

// TODO: think more about how we can setBuffer
// only once. Currently, we are resetting it on
// every play.
void AudioPlayer::play(const std::string &filePath)
{
    if (isPlaying())
    {
        return;
    }
    LOG_DEBUG(LogChannel::Audio, "Playing audio '{}' on source {}", filePath, m_source);
    unsigned int buffer = AssetManager::instance().getAudioClip(filePath);
    alSourcei(m_source, AL_BUFFER, buffer);
    alSourcePlay(m_source);
    checkAlError("AlSource play error");
}

void AudioPlayer::stop()
{
    alSourceStop(m_source);
}

void AudioPlayer::pause()
{
    alSourcePause(m_source);
}

void AudioPlayer::setLooping(bool loop)
{
    alSourcei(m_source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
}

void AudioPlayer::setGain(float gain)
{
    alSourcef(m_source, AL_GAIN, gain);
}

void AudioPlayer::setPitch(float pitch)
{
    alSourcef(m_source, AL_PITCH, pitch);
}

void AudioPlayer::setPosition(const glm::vec3 &pos)
{
    alSource3f(m_source, AL_POSITION, pos.x, pos.y, pos.z);
}

void AudioPlayer::setVelocity(const glm::vec3 &vel)
{
    alSource3f(m_source, AL_VELOCITY, vel.x, vel.y, vel.z);
}

void AudioPlayer::setRelative(bool relative)
{
    alSourcei(m_source, AL_SOURCE_RELATIVE, relative ? AL_TRUE : AL_FALSE);
}

bool AudioPlayer::isPlaying() const
{
    if (m_source == 0)
    {
        return false;
    }

    ALint state = AL_STOPPED;
    alGetSourcei(m_source, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}
