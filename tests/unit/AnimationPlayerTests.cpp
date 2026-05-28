#include "core/AnimationPlayer.h"

#include <gtest/gtest.h>

namespace
{
Animation makeAnimation()
{
    Animation animation;
    animation.name = "sway";
    Frame frame;
    frame.duration = 0.1f;
    animation.frames.push_back(frame);
    return animation;
}
} // namespace

TEST(AnimationPlayerTests, PlayResumesTheSamePausedAnimation)
{
    Animation animation = makeAnimation();
    AnimationPlayer player{animation};

    player.pause();
    ASSERT_FALSE(player.isPlaying());

    player.play(animation);
    EXPECT_TRUE(player.isPlaying());
    EXPECT_TRUE(player.hasAnimation());
}

TEST(AnimationPlayerTests, PlayRestartsTheSameStoppedAnimation)
{
    Animation animation = makeAnimation();
    AnimationPlayer player{animation};

    player.stop();
    ASSERT_FALSE(player.isPlaying());

    player.play(animation);
    EXPECT_TRUE(player.isPlaying());
    EXPECT_TRUE(player.hasAnimation());
}

TEST(AnimationPlayerTests, EmptyAnimationIsNotPlayable)
{
    Animation animation;
    animation.name = "empty";
    AnimationPlayer player;

    player.play(animation);

    EXPECT_FALSE(player.isPlaying());
    EXPECT_FALSE(player.hasAnimation());
}
