#pragma once

#include "core/Camera.h"
#include "core/FrameContext.h"
#include "core/SceneManager.h"
#include "core/Window.h"
#include "core/components/Camera.h"
#include "core/components/Transform.h"

#include "renderer/SpriteRenderer.h"
#include "util/Time.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

class BaseGame
{
public:
    virtual ~BaseGame() = default;

    virtual void onStart() = 0;
    virtual void onUpdate(const FrameContext &frame) = 0;
    virtual void onShutdown() = 0;

    // Entry point for the engine user (they call this, nothing else)
    void run()
    {
        onStart();
        runLoop();
        onShutdown();
    }

protected:
    void runLoop()
    {
        glfwSwapInterval(1);
        Time::reset();

        constexpr double fixedDeltaTime = 1.0 / 60.0;
        constexpr int maxStepsPerFrame = 8;
        double accumulatedTime = 0.0;
        double simulatedTime = 0.0;
        InputSnapshot pendingInput;

        while (!glfwWindowShouldClose(m_window.getGlfwWindow()))
        {
            Time::update();
            glfwPollEvents();
            const FrameContext frame = captureFrameContext(Time::deltaTime(), Time::elapsedTime());

            pendingInput.accumulate(frame.input);
            accumulatedTime += frame.deltaTime;

            int steps = 0;
            while (accumulatedTime >= fixedDeltaTime && steps < maxStepsPerFrame)
            {
                const FrameContext tick{
                    static_cast<float>(fixedDeltaTime), simulatedTime + fixedDeltaTime, pendingInput, frame.window};
                onUpdate(tick);
                m_sceneManager.update(tick);
                simulatedTime = tick.elapsedTime;
                accumulatedTime -= fixedDeltaTime;
                ++steps;

                // Later catch-up ticks keep held state without repeating edges or events.
                pendingInput.clearTransientState();
            }
            if (accumulatedTime >= fixedDeltaTime)
            {
                // Drop excess catch-up work while retaining time toward the next tick.
                accumulatedTime = std::fmod(accumulatedTime, fixedDeltaTime);
            }

            renderFrame(frame);

            glfwSwapBuffers(m_window.getGlfwWindow());
        }

        glfwTerminate();
        m_running = false;
    }

    // when changing consider if we should make similar updates
    // to editor layer loop as well
    void renderFrame(const FrameContext &frame)
    {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // FIXME: the whole rendering specific thing that is happening
        // here should be moved to a rendering system. It doesn't make
        // sense to process gameobject's camera and start rendering here.
        Scene &scene = m_sceneManager.getActiveScene();
        const auto &gameObjects = scene.getGameObjects();

        // Use the primary Camera component if one exists,
        GameObject *cameraObj = scene.findPrimaryCamera();
        if (cameraObj)
        {
            Camera &cam = cameraObj->getComponent<Camera>();
            const Transform &camTransform = cameraObj->getComponent<Transform>();

            float w = cam.logicalWidth * cam.zoom;
            float h = cam.logicalHeight * cam.zoom;
            float cameraAspect = w / h;

            // Letterbox/pillarbox: fit the camera's aspect ratio into the window
            auto [vpX, vpY, vpW, vpH] = frame.window.getViewportForAspect(cameraAspect);

            // FIXME: anything specific to glfw shouldn't be called here
            // Window.h should handle this as well
            glViewport(vpX, vpY, vpW, vpH);

            glm::mat4 proj = glm::ortho(0.0f, w, 0.0f, h, cam.nearClip, cam.farClip);
            glm::mat4 view = glm::translate(glm::mat4(1.0f), -camTransform.position);

            m_gameCamera.setProjectionMatrix(proj);
            m_gameCamera.setViewMatrix(view);

            // FIXME: the only reason we have m_gameCamera is because render
            // accepts CameraOld and not the Camera gameobject component.
            m_spriteRenderer.render(m_gameCamera, frame.elapsedTime, gameObjects);

        }
        else
        {
            throw std::runtime_error("No primary camera found!");
        }
    }

public:
    // for now these should be publicly available
    Window m_window;
    SceneManager m_sceneManager;
    SpriteRenderer m_spriteRenderer;
    CameraOld m_gameCamera;

    bool m_running = true;
};
