#pragma once
#include <AL/al.h>
#include <glm/vec3.hpp>
#include <string>

class AudioPlayer
{
public:
    // Sources share AudioResourceManager's process-wide context. System calls
    // that create, mutate, or destroy players declare AudioDevice writes.
    AudioPlayer();
    ~AudioPlayer();
    AudioPlayer(const AudioPlayer &) = delete;
    AudioPlayer &operator=(const AudioPlayer &) = delete;
    AudioPlayer(AudioPlayer &&other) noexcept;
    AudioPlayer &operator=(AudioPlayer &&other) noexcept;

    void play(const std::string &filePath);
    void stop();
    void pause();

    bool isPlaying() const;

    void setLooping(bool loop);
    void setGain(float gain);
    void setPitch(float pitch);
    void setPosition(const glm::vec3 &pos);
    void setVelocity(const glm::vec3 &vel);
    void setRelative(bool relative);

private:
    unsigned int m_source = 0;
};
