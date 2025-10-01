#include "core/AnimationPlayer.h"

#include "util/Time.h"

AnimationPlayer::AnimationPlayer(const Animation &animation)
{
    play(animation);
}

void AnimationPlayer::update()
{
    if (!m_playing)
        return;

    m_frameTimer += Time::deltaTime();
    ;
    const Frame &frame = mp_animation->frames[m_currentFrame];

    if (m_frameTimer >= frame.duration)
    {
        m_frameTimer = 0.0f;
        m_currentFrame++;
        if (m_currentFrame >= mp_animation->frames.size())
        {
            if (m_looping)
                m_currentFrame = 0;
            else
                m_currentFrame = mp_animation->frames.size() - 1;
        }
    }
}

void AnimationPlayer::play(const Animation &animation, bool loop)
{
    mp_animation = &animation;
    m_looping = loop;
    m_playing = true;
    m_currentFrame = 0;
    m_frameTimer = 0.0f;
}

void AnimationPlayer::pause()
{
    m_playing = false;
}
void AnimationPlayer::stop()
{
    m_playing = false;
    m_currentFrame = 0;
    m_frameTimer = 0.0f;
}

bool AnimationPlayer::isPlaying() const
{
    return m_playing;
}

const Frame &AnimationPlayer::getCurrentFrame() const
{
    return mp_animation->frames[m_currentFrame];
}
