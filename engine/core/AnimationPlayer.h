#pragma once
#include "core/components/Animation.h"

class AnimationPlayer
{
public:
    AnimationPlayer() = default;
    AnimationPlayer(const Animation &animation);

    // Advances playback using the caller's captured frame delta in seconds.
    void update(float deltaTime);
    const Frame &getCurrentFrame() const;

    // Borrows animation frames, which must remain alive and unchanged while
    // this player uses them. Animator retains the loaded AnimationMap owner.
    void play(const Animation &animation, bool loop = true);
    void pause();
    void stop();
    bool isPlaying() const;
    // TODO: only used in game object editor. Think more about it.
    bool hasAnimation() const;

private:
    const Animation *mp_animation = nullptr;
    size_t m_currentFrame = 0;

    float m_frameTimer = 0.0f;
    bool m_looping = true;
    bool m_playing = false;
};
