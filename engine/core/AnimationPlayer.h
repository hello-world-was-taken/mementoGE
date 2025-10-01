#pragma once
#include "core/Animation.h"

class AnimationPlayer
{
public:
    AnimationPlayer() = default;
    AnimationPlayer(const Animation &animation);

    // update should be called every frame
    // TODO: should we pass deltaTime or keep on using Time::deltaTime()
    void update();
    const Frame &getCurrentFrame() const;

    void play(const Animation &animation, bool loop = true);
    void pause();
    void stop();
    bool isPlaying() const;

private:
    const Animation *mp_animation = nullptr;
    size_t m_currentFrame = 0;

    float m_frameTimer = 0.0f;
    bool m_looping = true;
    bool m_playing = false;
};
