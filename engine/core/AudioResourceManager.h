#pragma once

#include <AL/al.h>
#include <AL/alc.h>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

#pragma pack(push, 1)
// A WAV header is a binary format with fields packed back to back.
// Compilers normally insert padding into structs to align integers,
// so without packing, WavHeader in memory != Wav header on disk struct WavHeader
struct WavHeader
{
    char riff[4];
    uint32_t chunkSize;
    char wave[4];

    char fmt[4];
    uint32_t subchunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;

    char data[4];
    uint32_t dataSize;
};
#pragma pack(pop)

class AudioResourceManager
{
public:
    // One device/context owns sources and cached buffers across all scenes.
    // System access is serialized by AudioDevice; owner access stays outside
    // worker batches, and all AudioPlayers must die before this singleton.
    static AudioResourceManager &instance();

    unsigned int getAudioBuffer(const std::string &audioPath);

private:
    AudioResourceManager();
    ~AudioResourceManager();

    AudioResourceManager(const AudioResourceManager &) = delete;
    AudioResourceManager &operator=(const AudioResourceManager &) = delete;

    struct DeviceDeleter
    {
        void operator()(ALCdevice *device) const;
    };

    struct ContextDeleter
    {
        void operator()(ALCcontext *context) const;
    };

    // Declared in this order so the context is destroyed before its device.
    std::unique_ptr<ALCdevice, DeviceDeleter> m_device;
    std::unique_ptr<ALCcontext, ContextDeleter> m_context;
    std::map<std::string, unsigned int> bufferByPath;

    unsigned int loadWav(const std::string &audioPath);
};
