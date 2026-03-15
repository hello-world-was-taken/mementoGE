#pragma once

#include "core/Camera.h"
#include "core/SceneManager.h"
#include "core/Window.h"
#include "core/components/Camera.h"
#include "core/components/Transform.h"

#include "renderer/SpriteRenderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <string>

class BaseGame
{
public:
    virtual ~BaseGame() = default;

    virtual void onStart() = 0;
    virtual void onUpdate() = 0;
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

        while (!glfwWindowShouldClose(m_window.getGlfwWindow()))
        {
            Time::update();
            glfwPollEvents();

            updateFrame();

            glfwSwapBuffers(m_window.getGlfwWindow());
        }

        glfwTerminate();
        m_running = false;
    }

    // when changing consider if we should make similar updates
    // to editor layer loop as well
    void updateFrame()
    {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // User logic
        onUpdate();

        // Engine logic
        m_sceneManager.update();

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
            Transform &transform = cameraObj->getComponent<Transform>();

            float w = cam.logicalWidth * cam.zoom;
            float h = cam.logicalHeight * cam.zoom;

            // Letterbox/pillarbox: fit the camera's aspect ratio into the window
            int fbWidth, fbHeight;
            glfwGetFramebufferSize(m_window.getGlfwWindow(), &fbWidth, &fbHeight);

            float cameraAspect = w / h;
            float windowAspect = static_cast<float>(fbWidth) / static_cast<float>(fbHeight);

            int vpX = 0, vpY = 0, vpW = fbWidth, vpH = fbHeight;
            if (cameraAspect > windowAspect)
            {
                // Camera wider than window: pillarbox (bars top/bottom)
                vpW = fbWidth;
                vpH = static_cast<int>(fbWidth / cameraAspect);
                vpY = (fbHeight - vpH) / 2;
            }
            else
            {
                // Camera taller than window: letterbox (bars left/right)
                vpH = fbHeight;
                vpW = static_cast<int>(fbHeight * cameraAspect);
                vpX = (fbWidth - vpW) / 2;
            }

            glViewport(vpX, vpY, vpW, vpH);

            glm::mat4 proj = glm::ortho(0.0f, w, 0.0f, h, cam.nearClip, cam.farClip);
            glm::mat4 view = glm::translate(glm::mat4(1.0f), -transform.position);

            m_gameCamera.setProjectionMatrix(proj);
            m_gameCamera.setViewMatrix(view);

            // FIXME: the only reason we have m_gameCamera is because render
            // accepts CameraOld and not the Camera gameobject component.
            m_spriteRenderer.render(m_gameCamera, gameObjects);
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
