#include "core/AnimationPlayer.h"

#include <gtest/gtest.h>

namespace
{
Animation makeAnimation(std::size_t frameCount = 1)
{
    Animation animation;
    animation.name = "sway";
    Frame frame;
    frame.duration = 0.1f;
    animation.frames.assign(frameCount, frame);
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

TEST(AnimationPlayerTests, AdvancesWhenSuppliedDeltaAccumulatesToFrameDuration)
{
    Animation animation = makeAnimation(3);
    AnimationPlayer player{animation};

    player.update(0.04f);
    player.update(0.04f);
    EXPECT_EQ(&player.getCurrentFrame(), &animation.frames[0]);

    player.update(0.03f);
    EXPECT_EQ(&player.getCurrentFrame(), &animation.frames[1]);

    player.update(0.0f);
    EXPECT_EQ(&player.getCurrentFrame(), &animation.frames[1]);
}

TEST(AnimationPlayerTests, IndependentPlayersUseTheirOwnSuppliedDelta)
{
    Animation animation = makeAnimation(3);
    AnimationPlayer slowPlayer{animation};
    AnimationPlayer fastPlayer{animation};

    slowPlayer.update(0.02f);
    fastPlayer.update(0.11f);

    EXPECT_EQ(&slowPlayer.getCurrentFrame(), &animation.frames[0]);
    EXPECT_EQ(&fastPlayer.getCurrentFrame(), &animation.frames[1]);
}

TEST(AnimationPlayerTests, PausedUpdatesDoNotAccumulateSuppliedDelta)
{
    Animation animation = makeAnimation(3);
    AnimationPlayer player{animation};

    player.update(0.04f);
    player.pause();
    player.update(1.0f);
    EXPECT_EQ(&player.getCurrentFrame(), &animation.frames[0]);

    player.play(animation);
    player.update(0.03f);
    EXPECT_EQ(&player.getCurrentFrame(), &animation.frames[0]);

    player.update(0.04f);
    EXPECT_EQ(&player.getCurrentFrame(), &animation.frames[1]);
}
