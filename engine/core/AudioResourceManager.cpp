#include "AudioResourceManager.h"

#include <fstream>
#include <stdexcept>
#include <vector>

AudioResourceManager &AudioResourceManager::instance()
{
    static AudioResourceManager resourceManager;
    return resourceManager;
}

AudioResourceManager::AudioResourceManager() : m_device(alcOpenDevice(nullptr))
{
    if (!m_device)
    {
        throw std::runtime_error("Failed to open OpenAL device");
    }

    m_context.reset(alcCreateContext(m_device.get(), nullptr));
    if (!m_context)
    {
        throw std::runtime_error("Failed to create OpenAL context");
    }
    if (alcMakeContextCurrent(m_context.get()) != ALC_TRUE)
    {
        throw std::runtime_error("Failed to make OpenAL context current");
    }
}

void AudioResourceManager::DeviceDeleter::operator()(ALCdevice *device) const
{
    if (device)
    {
        alcCloseDevice(device);
    }
}

void AudioResourceManager::ContextDeleter::operator()(ALCcontext *context) const
{
    if (context)
    {
        if (alcGetCurrentContext() == context)
        {
            alcMakeContextCurrent(nullptr);
        }
        alcDestroyContext(context);
    }
}

AudioResourceManager::~AudioResourceManager()
{
    // AudioPlayers have already released their sources. Delete buffers while
    // their owning context is alive; the RAII members then destroy the context
    // and device. The members also clean up a failed initialization.
    alcMakeContextCurrent(m_context.get());
    for (auto &[_, buffer] : bufferByPath)
    {
        alDeleteBuffers(1, &buffer);
    }
}

unsigned int AudioResourceManager::getAudioBuffer(const std::string &audioPath)
{
    auto it = bufferByPath.find(audioPath);
    if (it != bufferByPath.end())
    {
        return it->second;
    }

    unsigned int buffer = loadWav(audioPath);
    bufferByPath[audioPath] = buffer;
    return buffer;
}

unsigned int AudioResourceManager::loadWav(const std::string &audioPath)
{
    std::ifstream file(audioPath, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Failed to open WAV file: " + audioPath);
    }

    WavHeader header{};
    file.read(reinterpret_cast<char *>(&header), sizeof(header));

    if (std::string(header.riff, 4) != "RIFF" || std::string(header.wave, 4) != "WAVE")
    {
        throw std::runtime_error("Invalid WAV file: " + audioPath);
    }

    // FIXME: using a vector<char> might be better here. That way we'll always have
    // access to size as well and no need to use header.dataSize when generating buffers
    // Update other places unique_ptr is used to load resources as well.
    std::unique_ptr<char[]> data = std::make_unique<char[]>(header.dataSize);
    file.read(data.get(), header.dataSize);

    ALenum format;
    if (header.numChannels == 1 && header.bitsPerSample == 8)
    {
        format = AL_FORMAT_MONO8;
    }
    else if (header.numChannels == 1 && header.bitsPerSample == 16)
    {
        format = AL_FORMAT_MONO16;
    }
    else if (header.numChannels == 2 && header.bitsPerSample == 8)
    {
        format = AL_FORMAT_STEREO8;
    }
    else if (header.numChannels == 2 && header.bitsPerSample == 16)
    {
        format = AL_FORMAT_STEREO16;
    }
    else
    {
        throw std::runtime_error("Unsupported WAV format: " + audioPath);
    }

    ALuint buffer;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, format, data.get(), header.dataSize, header.sampleRate);

    return buffer;
}
