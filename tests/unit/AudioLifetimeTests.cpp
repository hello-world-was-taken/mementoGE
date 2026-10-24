#include "core/AssetManager.h"
#include "core/AudioPlayer.h"
#include "core/AudioResourceManager.h"
#include "core/AudioSystem.h"

#include <AL/alc.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>

#include <gtest/gtest.h>

namespace
{
class TemporaryWav
{
public:
    TemporaryWav()
        : path(std::filesystem::temp_directory_path() /
              ("memento-audio-lifetime-" +
                  std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".wav"))
    {
        const std::array<std::int16_t, 80> samples{};
        WavHeader header{};
        std::memcpy(header.riff, "RIFF", 4);
        std::memcpy(header.wave, "WAVE", 4);
        std::memcpy(header.fmt, "fmt ", 4);
        std::memcpy(header.data, "data", 4);
        header.chunkSize = 36 + sizeof(samples);
        header.subchunk1Size = 16;
        header.audioFormat = 1;
        header.numChannels = 1;
        header.sampleRate = 8000;
        header.byteRate = 16000;
        header.blockAlign = 2;
        header.bitsPerSample = 16;
        header.dataSize = sizeof(samples);
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char *>(&header), sizeof(header));
        file.write(reinterpret_cast<const char *>(samples.data()), sizeof(samples));
        if (!file)
        {
            throw std::runtime_error("Could not write test WAV");
        }
    }

    ~TemporaryWav()
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    const std::filesystem::path path;
};
} // namespace

TEST(AudioLifetimeTests, SourcesAndCachedBuffersKeepOneContextAcrossSystemLifetimes)
{
    // CTest selects OpenAL Soft's null backend; no physical audio device is used.
    TemporaryWav wav;
    AudioPlayer player;
    ALCcontext *context = alcGetCurrentContext();
    ASSERT_NE(context, nullptr);
    ASSERT_EQ(alGetError(), AL_NO_ERROR);

    auto firstSceneSystem = std::make_unique<AudioSystem>();
    auto secondSceneSystem = std::make_unique<AudioSystem>();
    EXPECT_EQ(alcGetCurrentContext(), context);

    player.setLooping(true);
    player.play(wav.path.string());
    const auto buffer = AssetManager::instance().getAudioClip(wav.path.string());
    EXPECT_TRUE(alIsBuffer(buffer));
    EXPECT_TRUE(player.isPlaying());
    EXPECT_EQ(alGetError(), AL_NO_ERROR);

    firstSceneSystem.reset();
    EXPECT_EQ(alcGetCurrentContext(), context);
    EXPECT_TRUE(player.isPlaying());
    secondSceneSystem.reset();
    EXPECT_EQ(alcGetCurrentContext(), context);

    player.stop();
    AssetManager::instance().forgetAsset(wav.path.string());
    EXPECT_EQ(AssetManager::instance().getAudioClip(wav.path.string()), buffer);
    player.play(wav.path.string());
    EXPECT_TRUE(player.isPlaying());
    EXPECT_EQ(alGetError(), AL_NO_ERROR);
}
