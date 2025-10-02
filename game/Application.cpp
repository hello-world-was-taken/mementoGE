#include <memory>
#include <iostream>

#include "Application.h"
#include "engine/core/GameObject.h"
#include "engine/core/Window.h"
#include "engine/core/MouseListener.h"
#include "engine/core/SceneManager.h"
#include "engine/core/Scene.h"
#include "engine/core/Sprite.h"
#include "engine/core/Event.h"

#include "util/Time.h"

Application::Application(bool editorMode)
    : // using 16:9 for window size to match our virtual screen setup
      mWindow{1280, 720},
      mSceneManager{&mWindow},
      m_editorMode{editorMode},
      m_editorLayer{mWindow}
{
}

Application::~Application()
{
}

void Application::setup()
{
    if (m_editorMode)
    {
        m_editorLayer.prepare();
    }
    else
    {
        mSceneManager.deserialize();
        mSceneManager.prepare();
    }
}

void Application::start()
{
    setup();
    processInput();
    update();
}

void Application::processInput()
{
    if (m_editorMode)
    {
        mWindow.setupCallBack();
    }
}

void Application::update()
{
    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(mWindow.getGlfwWindow()))
    {
        Time::update();
        glfwPollEvents();

        if (m_editorMode)
        {
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            m_editorLayer.update();
            MouseListener::get()->beginFrame();

            // can we neatly wrap this in a function
            // End frame and render
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                GLFWwindow *backup_current_context = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(backup_current_context);
            }
        }
        else
        {
            mSceneManager.update();
        }

        glfwSwapBuffers(mWindow.getGlfwWindow());
    }

    destroy();

    glfwTerminate();
}

void Application::destroy()
{
}