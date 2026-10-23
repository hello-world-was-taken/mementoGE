#include "engine/core/CameraUtils.h"

#ifdef EDITOR_BUILD
#include "engine/editor/EditorCamera.h"
#endif

#include <array>
#include <gtest/gtest.h>

namespace
{
void expectNdc(const Camera &camera, glm::vec3 world, glm::vec3 expected)
{
    const glm::vec4 clip = camera.projection * camera.view * glm::vec4(world, 1.0f);
    ASSERT_NE(clip.w, 0.0f);
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    EXPECT_NEAR(ndc.x, expected.x, 0.0001f);
    EXPECT_NEAR(ndc.y, expected.y, 0.0001f);
    EXPECT_NEAR(ndc.z, expected.z, 0.0001f);
}
} // namespace

TEST(CameraTests, MatricesMapTranslatedVisibleBoundsAndClipPlanesToNdc)
{
    Camera camera;
    camera.logicalWidth = 320.0f;
    camera.logicalHeight = 180.0f;
    camera.zoom = 1.5f;
    camera.nearClip = -20.0f;
    camera.farClip = 60.0f;
    Transform transform;
    transform.position = {37.0f, -23.0f, 6.0f};

    CameraUtils::updateMatrices(camera, transform);

    expectNdc(camera, {37.0f, -23.0f, 26.0f}, {-1.0f, -1.0f, -1.0f});
    expectNdc(camera, {517.0f, 247.0f, -54.0f}, {1.0f, 1.0f, 1.0f});
    expectNdc(camera, {277.0f, 112.0f, -14.0f}, {0.0f, 0.0f, 0.0f});
}

TEST(CameraTests, UpdatingMatricesReplacesThePreviousZoomPositionAndClipRange)
{
    Camera camera;
    camera.logicalWidth = 200.0f;
    camera.logicalHeight = 100.0f;
    camera.nearClip = -10.0f;
    camera.farClip = 10.0f;
    Transform transform;
    transform.position = {0.0f, 0.0f, 0.0f};
    CameraUtils::updateMatrices(camera, transform);
    expectNdc(camera, {100.0f, 50.0f, 0.0f}, {0.0f, 0.0f, 0.0f});

    camera.zoom = 0.5f;
    camera.nearClip = -5.0f;
    camera.farClip = 15.0f;
    transform.position = {-40.0f, 80.0f, 3.0f};
    CameraUtils::updateMatrices(camera, transform);

    expectNdc(camera, {-40.0f, 80.0f, 8.0f}, {-1.0f, -1.0f, -1.0f});
    expectNdc(camera, {60.0f, 130.0f, -12.0f}, {1.0f, 1.0f, 1.0f});
    expectNdc(camera, {10.0f, 105.0f, -2.0f}, {0.0f, 0.0f, 0.0f});
}

TEST(CameraTests, ScreenToWorldAgreesWithSharedMatricesAcrossHighDpiLetterboxing)
{
    const WindowMetrics window{900, 700, 1800, 1400, 2.0f, 2.0f};
    Camera camera;
    camera.logicalWidth = 200.0f;
    camera.logicalHeight = 100.0f;
    camera.zoom = 1.75f;
    Transform transform;
    transform.position = {23.0f, -18.0f, 4.0f};
    CameraUtils::updateMatrices(camera, transform);
    const auto viewport = window.getViewportForAspect(camera.logicalWidth / camera.logicalHeight);
    ASSERT_GT(viewport.y, 0);
    const std::array<glm::vec2, 4> points{{{23.0f, -18.0f}, {373.0f, 157.0f},
        {198.0f, 69.5f}, {58.0f, 113.25f}}};

    for (const auto &point : points)
    {
        const glm::vec4 clip = camera.projection * camera.view * glm::vec4(point, transform.position.z, 1.0f);
        const glm::vec2 ndc = glm::vec2(clip) / clip.w;
        const float framebufferX = viewport.x + (ndc.x + 1.0f) * 0.5f * viewport.width;
        const float framebufferY = viewport.y + (ndc.y + 1.0f) * 0.5f * viewport.height;
        const glm::vec2 screen{
            framebufferX / window.framebufferWidth * window.width,
            (1.0f - framebufferY / window.framebufferHeight) * window.height};

        const auto restored = CameraUtils::screenToWorld(window, camera, transform, screen);

        EXPECT_NEAR(restored.x, point.x, 0.001f);
        EXPECT_NEAR(restored.y, point.y, 0.001f);
    }
}

#ifdef EDITOR_BUILD
TEST(EditorCameraTests, NavigationKeepsSharedCameraAndTransformMatricesCurrent)
{
    EditorCamera editor;
    EXPECT_FALSE(editor.camera().primary);
    editor.onViewportResize(1000.0f, 500.0f);
    EXPECT_FLOAT_EQ(editor.camera().logicalWidth, 400.0f);
    EXPECT_FLOAT_EQ(editor.camera().logicalHeight, 200.0f);
    expectNdc(editor.camera(), {400.0f, 200.0f, 0.0f}, {1.0f, 1.0f, 0.0f});

    editor.setPosition({30.0f, -10.0f, 4.0f});
    expectNdc(editor.camera(), {30.0f, -10.0f, 4.0f}, {-1.0f, -1.0f, 0.0f});
    editor.update(0.25f, {8.0f, -4.0f});
    EXPECT_FLOAT_EQ(editor.getPosition().x, 32.0f);
    EXPECT_FLOAT_EQ(editor.getPosition().y, -11.0f);
    EXPECT_FLOAT_EQ(editor.transform().position.z, 4.0f);
    expectNdc(editor.camera(), {32.0f, -11.0f, 4.0f}, {-1.0f, -1.0f, 0.0f});

    editor.adjustZoom(5.0f);
    EXPECT_FLOAT_EQ(editor.getZoom(), 0.5f);
    expectNdc(editor.camera(), {232.0f, 89.0f, 4.0f}, {1.0f, 1.0f, 0.0f});
    editor.adjustZoom(10000.0f);
    EXPECT_FLOAT_EQ(editor.getZoom(), editor.camera().minZoom);
    expectNdc(editor.camera(), {72.0f, 9.0f, 4.0f}, {1.0f, 1.0f, 0.0f});
    editor.adjustZoom(-10000.0f);
    EXPECT_FLOAT_EQ(editor.getZoom(), editor.camera().maxZoom);
    expectNdc(editor.camera(), {4032.0f, 1989.0f, 4.0f}, {1.0f, 1.0f, 0.0f});
}
#endif
