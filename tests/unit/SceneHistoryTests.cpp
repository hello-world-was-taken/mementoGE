#include "engine/core/components/Transform.h"
#include "engine/editor/SceneHistory.h"

#include <gtest/gtest.h>

namespace
{
GameObject &firstObject(Scene &scene)
{
    return scene.getGameObjects().front();
}
} // namespace

TEST(SceneHistoryTests, UndoAndRedoRestoreSceneSnapshots)
{
    Scene scene{"history_test"};
    GameObject &object = scene.addGameObject(16, 16, "crate");
    object.getComponent<Transform>().position = {1.0f, 2.0f, 0.0f};

    SceneHistory history;
    history.pushInitialScene(scene);

    object.getComponent<Transform>().position = {10.0f, 20.0f, 0.0f};
    history.pushSnapshot(scene);

    ASSERT_TRUE(history.undo(scene));
    EXPECT_FLOAT_EQ(firstObject(scene).getComponent<Transform>().position.x, 1.0f);
    EXPECT_FLOAT_EQ(firstObject(scene).getComponent<Transform>().position.y, 2.0f);

    ASSERT_TRUE(history.redo(scene));
    EXPECT_FLOAT_EQ(firstObject(scene).getComponent<Transform>().position.x, 10.0f);
    EXPECT_FLOAT_EQ(firstObject(scene).getComponent<Transform>().position.y, 20.0f);
}

TEST(SceneHistoryTests, ConsecutiveIdenticalSnapshotsAreIgnored)
{
    Scene scene{"history_test"};
    scene.addGameObject(16, 16, "crate");

    SceneHistory history;
    history.pushInitialScene(scene);
    history.pushSnapshot(scene);
    history.pushSnapshot(scene);

    EXPECT_EQ(history.getStackSize(), 1u);
    EXPECT_FALSE(history.canUndo());

    firstObject(scene).getComponent<Transform>().position.x = 5.0f;
    history.pushSnapshot(scene);
    EXPECT_EQ(history.getStackSize(), 2u);
    EXPECT_TRUE(history.canUndo());
}

TEST(SceneHistoryTests, EditingAfterUndoClearsRedoHistory)
{
    Scene scene{"history_test"};
    GameObject &object = scene.addGameObject(16, 16, "crate");

    SceneHistory history;
    history.pushInitialScene(scene);
    object.getComponent<Transform>().position.x = 5.0f;
    history.pushSnapshot(scene);
    object.getComponent<Transform>().position.x = 10.0f;
    history.pushSnapshot(scene);

    ASSERT_TRUE(history.undo(scene));
    firstObject(scene).getComponent<Transform>().position.x = 7.0f;
    history.pushSnapshot(scene);

    EXPECT_FALSE(history.canRedo());
    ASSERT_TRUE(history.undo(scene));
    EXPECT_FLOAT_EQ(firstObject(scene).getComponent<Transform>().position.x, 5.0f);
}
