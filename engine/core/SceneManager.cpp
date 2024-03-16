#include <yaml-cpp/yaml.h>

#include "engine/core/SceneManager.h"
#include "engine/core/ImGuiWrapper.h"
#include "engine/core/Resource.h"
#include "engine/core/Sprite.h"

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

    this->serialize();
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
    // TODO: shouldn't the active scene be set to the latest scene added?
    if (m_activeScene == nullptr)
    {
        m_activeScene = m_scenes[sceneName];
    }
}

void SceneManager::removeScene(const char *sceneName)
{
    m_scenes.erase(sceneName);
}

void SceneManager::renderTextureResourcesImGui()
{
    for (auto &[texturePath, texture] : *Resource::getTexturesMap().get())
    {
        ImGui::Text("Sprites");
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
    }
}

std::shared_ptr<Scene> SceneManager::getActiveScene()
{
    return m_activeScene;
}

void SceneManager::serialize()
{
    YAML::Emitter out;
    out << YAML::BeginMap;

    // iterate through the map of scenes
    for (auto &[sceneName, scene] : m_scenes)
    {
        out << YAML::Key << sceneName;
        out << YAML::Value << YAML::BeginMap;

        int count = 0; // TESTING: remove this
        for (auto gameObject : *scene.get()->getGameObjects().get())
        {
            std::string gameObjectName = "GameObject" + std::to_string(count);
            out << YAML::Key << gameObjectName;
            out << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Width";
            out << YAML::Value << gameObject.get()->getWidth();
            out << YAML::Key << "Height";
            out << YAML::Value << gameObject.get()->getHeight();

            // TRANSFORM COMPONENT
            if (gameObject->hasComponent<Transform>())
            {
                Transform &transform = gameObject->getComponent<Transform>();
                transform.serialize(out);
            }

            // SPRITE COMPONENT
            if (gameObject->hasComponent<Sprite>())
            {
                Sprite &sprite = gameObject->getComponent<Sprite>();
                sprite.serialize(out);
            }

            out << YAML::EndMap;
            count++;
        }

        out << YAML::EndMap;
    }

    out << YAML::EndMap;
    std::ofstream file("../game/scene.yaml", std::ios::out | std::ios::trunc);
    file << out.c_str();

    std::cout << "Serialized scene to scene.yaml" << std::endl;
}

void SceneManager::deserialize()
{
    // TODO: if we update our serialization method and scene.yaml doesn't adhere to that
    //       we should throw an error or remove the file
    // if a file called ../game/scene.yaml does not exist, return
    if (!std::filesystem::exists("../game/scene.yaml"))
    {
        std::cout << "scene.yaml does not exist" << std::endl;
        return;
    }

    YAML::Node scene = YAML::LoadFile("../game/scene.yaml");
    for (auto it = scene.begin(); it != scene.end(); ++it) // why ++it instead of it++?
    {
        std::string sceneName = it->first.as<std::string>();
        std::shared_ptr<Scene> newScene = std::make_shared<Scene>();
        m_scenes[sceneName] = newScene;
        m_activeScene = m_scenes[sceneName]; // TODO: have some sort of active scene flag

        for (auto serializedGameObject : it->second)
        {
            unsigned int width = serializedGameObject.second["Width"].as<unsigned int>();
            unsigned int height = serializedGameObject.second["Height"].as<unsigned int>();

            std::shared_ptr<GameObject> newGameObject = m_activeScene->addGameObject(width, height);
            // Deserialize Transform Component
            newGameObject->addComponent<Transform>(glm::vec3(0.0f, 0.0f, 0.0f));
            newGameObject->getComponent<Transform>().deserialize(serializedGameObject.second);

            // Deserialize Sprite Component
            newGameObject->addComponent<Sprite>();
            newGameObject->getComponent<Sprite>().deserialize(serializedGameObject.second);
        }
    }
    std::cout << "Deserialized scene from scene.yaml" << std::endl;
}