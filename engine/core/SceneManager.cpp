#include "engine/core/SceneManager.h"
#include "engine/core/ImGuiWrapper.h"

SceneManager::SceneManager(Window *m_window)
{
    this->m_window = m_window;
    // this->addScene("triangle_scene", std::make_shared<Scene>());
    // m_activeScene = m_scenes.at("triangle_scene");
}

SceneManager::~SceneManager()
{
    std::cout << "SceneManager destructor called" << std::endl;
}

void SceneManager::start()
{
    if (m_activeScene == nullptr)
    {
        std::cout << "No active m_scene found" << std::endl;
        return;
    }
    m_activeScene->start(this->m_window->getGlfwWindow());
}

void SceneManager::update(float deltaTime)
{
    if (m_activeScene == nullptr)
    {
        std::cout << "No active m_scene found" << std::endl;
        return;
    }
    m_eventHandlerFunction(m_window->getGlfwWindow(), this);
    m_activeScene->update(deltaTime, m_window->getGlfwWindow());
}

void SceneManager::gameLoop()
{
    std::cout << "Drawing our scene" << std::endl;
    ImVec4 clear_color = ImVec4(0.5f, 0.5f, 0.5f, 1.00f);

    ImGuiWrapper::setupImgui(m_window->getGlfwWindow());
    glfwSwapInterval(1);
    // start scene manager by loading the first scene
    this->start();

    while (!glfwWindowShouldClose(m_window->getGlfwWindow()))
    {
        glfwPollEvents();
        ImGuiWrapper::showImguiDemo();
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);

        this->update(Time::deltaTime());

        // Rendering
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // TODO: why have a static Time class if we are going to pass deltaTime to the update function?
        Time::update();
        glfwSwapBuffers(m_window->getGlfwWindow());
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}

void SceneManager::setEventHandler(EventHandlerFunction eventHandler)
{
    m_eventHandlerFunction = eventHandler;
}

void SceneManager::loadScene(const char *sceneName)
{
    auto it = m_scenes.find(sceneName);
    if (it != m_scenes.end())
    {
        m_activeScene = it->second;
        m_activeScene->start(m_window->getGlfwWindow());
    }
    else
    {
        std::cout << "Scene not found: " << sceneName << std::endl;
    }
}

void SceneManager::unloadScene(const char *sceneName)
{
    m_activeScene = nullptr;
}

void SceneManager::addScene(const char *sceneName, std::shared_ptr<Scene> m_scene)
{
    m_scenes[sceneName] = m_scene;
    if (m_activeScene == nullptr)
    {
        m_activeScene = m_scenes[sceneName];
    }
}

void SceneManager::removeScene(const char *sceneName)
{
    m_scenes.erase(sceneName);
}

std::shared_ptr<Scene> SceneManager::getActiveScene()
{
    return m_activeScene;
}