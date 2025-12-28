#include "core/AudioPlayer.h"
#include "core/AssetManager.h"

#include "util/log_error.h"

#include <iostream>

AudioPlayer::AudioPlayer()
{
    alGenSources(1, &m_source);
    checkAlError("alGenSources");
}

AudioPlayer::~AudioPlayer()
{
    alDeleteSources(1, &m_source);
}

// void AudioPlayer::setBuffer(ALuint buffer)
// {
//     alSourcei(m_source, AL_BUFFER, buffer);
// }

// TODO: think more about how we can setBuffer
// only once. Currently, we are resetting it on
// every play.
void AudioPlayer::play(std::string &filePath)
{
    if (isPlaying())
    {
        return;
    }
    std::cout << "Playing audio: " << filePath << std::endl;
    std::cout << "m_source: " << m_source << std::endl;
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

bool AudioPlayer::isPlaying() const
{
    ALint state;
    alGetSourcei(m_source, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}
