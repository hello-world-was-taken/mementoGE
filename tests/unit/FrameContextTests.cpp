#include "engine/core/CameraUtils.h"
#include "engine/core/Scene.h"
#include "engine/core/SystemRegistry.h"

#include <gtest/gtest.h>
#include <type_traits>
#include <vector>

namespace
{
struct Observation
{
    const FrameContext *frame;
    float deltaTime;
    double elapsedTime;
    bool pressed;
    std::size_t events;
};

std::vector<Observation> observations;

template <int Id> class FrameReader final : public ISystem
{
public:
    void update(SystemContext &context) override
    {
        bool pressed = false;
        for (const Event &event : context.frame.input.events)
        {
            if (event.type == EventType::KeyPress && event.keyType == KeyType::Space)
            {
                pressed = true;
            }
        }
        observations.push_back({&context.frame,
            context.frame.deltaTime,
            context.frame.elapsedTime,
            pressed,
            context.frame.input.events.size()});
    }

    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("FrameContextTests.Reader" + std::to_string(Id));
    }
};

static_assert(std::is_const_v<std::remove_reference_t<decltype(SystemContext::frame)>>,
    "Systems must receive a const frame snapshot");
} // namespace

TEST(FrameContextTests, AllSystemsReadTheSameFrameWithoutConsumingEvents)
{
    SystemRegistry::instance().registerSystem<FrameReader<1>>("FrameContextTests.Reader1");
    SystemRegistry::instance().registerSystem<FrameReader<2>>("FrameContextTests.Reader2");
    Scene scene{"frame readers"};
    scene.addSystem("FrameContextTests.Reader1");
    scene.addSystem("FrameContextTests.Reader2");
    scene.setAllSystemsEnabled(true);
    scene.play();

    FrameContext frame;
    frame.deltaTime = 0.125f;
    frame.elapsedTime = 15.75;
    Event event{};
    event.type = EventType::KeyPress;
    event.keyType = KeyType::Space;
    frame.input.events.push_back(event);
    observations.clear();
    scene.update(frame);

    ASSERT_EQ(observations.size(), 2u);
    for (const auto &observed : observations)
    {
        EXPECT_EQ(observed.frame, &frame);
        EXPECT_FLOAT_EQ(observed.deltaTime, 0.125f);
        EXPECT_DOUBLE_EQ(observed.elapsedTime, 15.75);
        EXPECT_TRUE(observed.pressed);
        EXPECT_EQ(observed.events, 1u);
    }

    // A supplied snapshot can be replayed without a live window, clock, or callbacks.
    scene.update(frame);
    ASSERT_EQ(observations.size(), 4u);
    EXPECT_TRUE(observations.back().pressed);
    EXPECT_EQ(observations.back().events, 1u);
}

TEST(FrameContextTests, WindowMetricsFitViewportAndConvertHighDpiCoordinates)
{
    const WindowMetrics window{800, 600, 1600, 1200, 2.0f, 2.0f};
    const auto viewport = window.getViewportForAspect(2.0f);
    EXPECT_EQ(viewport.x, 0);
    EXPECT_EQ(viewport.y, 200);
    EXPECT_EQ(viewport.width, 1600);
    EXPECT_EQ(viewport.height, 800);

    Camera camera;
    camera.logicalWidth = 100.0f;
    camera.logicalHeight = 50.0f;
    camera.zoom = 1.0f;
    Transform transform;
    transform.position = {10.0f, 20.0f, 0.0f};
    const auto center = CameraUtils::screenToWorld(window, camera, transform, {400.0f, 300.0f});
    EXPECT_NEAR(center.x, 60.0f, 0.001f);
    EXPECT_NEAR(center.y, 45.0f, 0.001f);
    const auto topLeft = CameraUtils::screenToWorld(window, camera, transform, {0.0f, 100.0f});
    EXPECT_NEAR(topLeft.x, 10.0f, 0.001f);
    EXPECT_NEAR(topLeft.y, 70.0f, 0.001f);
}

TEST(FrameContextTests, MinimizedWindowHasAnEmptyViewportAndFiniteCoordinates)
{
    const WindowMetrics minimized{800, 600, 0, 0};
    EXPECT_EQ(minimized.getViewportForAspect(2.0f).width, 0);
    EXPECT_EQ(minimized.getViewportForAspect(0.0f).height, 0);
    Transform transform;
    transform.position = {10.0f, 20.0f, 0.0f};
    const auto point = CameraUtils::screenToWorld(minimized, Camera{}, transform, {400.0f, 300.0f});
    EXPECT_FLOAT_EQ(point.x, 10.0f);
    EXPECT_FLOAT_EQ(point.y, 20.0f);
}
