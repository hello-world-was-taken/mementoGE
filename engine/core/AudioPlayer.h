#pragma once
#include <AL/al.h>
#include <glm/vec3.hpp>

class AudioPlayer
{
public:
    AudioPlayer();
    ~AudioPlayer();

    void play(std::string &filePath);
    void stop();
    void pause();

    bool isPlaying() const;

    void setLooping(bool loop);
    void setGain(float gain);
    void setPitch(float pitch);
    void setPosition(const glm::vec3 &pos);
    void setVelocity(const glm::vec3 &vel);

private:
    unsigned int m_source = 0;
};
