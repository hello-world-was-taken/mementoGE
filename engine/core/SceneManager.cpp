#include <yaml-cpp/yaml.h>

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

            out << YAML::Key << "Transform";
            out << YAML::Value << YAML::BeginMap;

            out << YAML::Key << "Position";
            out << YAML::Value << YAML::BeginSeq;
            out << gameObject.get()->getComponent<Transform>().getPosition()->x;
            out << gameObject.get()->getComponent<Transform>().getPosition()->y;
            out << gameObject.get()->getComponent<Transform>().getPosition()->z;
            out << YAML::EndSeq;

            out << YAML::Key << "Rotation";
            out << YAML::Value << YAML::BeginSeq;
            out << gameObject->getComponent<Transform>().getRotation()->x;
            out << gameObject->getComponent<Transform>().getRotation()->y;
            out << gameObject->getComponent<Transform>().getRotation()->z;
            out << YAML::EndSeq;

            out << YAML::Key << "Scale";
            out << YAML::Value << YAML::BeginSeq;
            out << gameObject->getComponent<Transform>().getScale()->x;
            out << gameObject->getComponent<Transform>().getScale()->y;
            out << gameObject->getComponent<Transform>().getScale()->z;
            out << YAML::EndSeq;

            out << YAML::EndMap;

            // TODO: not all game objects will have a sprite renderer
            out << YAML::Key << "SpriteRenderer";
            out << YAML::Value << YAML::BeginMap;

            // TODO: is there a need to change this?
            // I don't need to store texture coordinates as I use sprite width and hight along with
            // x and y index to calculate texture coordinates
            // out << YAML::Key << "TextureCoordinates";
            // out << YAML::Value << YAML::BeginSeq;
            // out << gameObject->getComponent<SpriteRenderer>().getTextureCoordinates()[0].x;
            // out << gameObject->getComponent<SpriteRenderer>().getTextureCoordinates()[0].y;
            // out << gameObject->getComponent<SpriteRenderer>().getTextureCoordinates()[1].x;
            // out << gameObject->getComponent<SpriteRenderer>().getTextureCoordinates()[1].y;
            // out << gameObject->getComponent<SpriteRenderer>().getTextureCoordinates()[2].x;
            // out << gameObject->getComponent<SpriteRenderer>().getTextureCoordinates()[2].y;
            // out << gameObject->getComponent<SpriteRenderer>().getTextureCoordinates()[3].x;
            // out << gameObject->getComponent<SpriteRenderer>().getTextureCoordinates()[3].y;
            // out << YAML::EndSeq;

            out << YAML::Key << "spriteWidth";
            out << YAML::Value << gameObject->getComponent<SpriteRenderer>().getSpriteWidth();
            out << YAML::Key << "spriteHeight";
            out << YAML::Value << gameObject->getComponent<SpriteRenderer>().getSpriteHeight();

            out << YAML::Key << "xIndex";
            out << YAML::Value << gameObject->getComponent<SpriteRenderer>().getXIndex();
            out << YAML::Key << "yIndex";
            out << YAML::Value << gameObject->getComponent<SpriteRenderer>().getYIndex();

            // TODO: some sprites might not have a texture only color and vise versa
            out << YAML::Key << "Color";
            out << YAML::Value << YAML::BeginSeq;
            out << gameObject->getComponent<SpriteRenderer>().getColor().r;
            out << gameObject->getComponent<SpriteRenderer>().getColor().g;
            out << gameObject->getComponent<SpriteRenderer>().getColor().b;
            out << gameObject->getComponent<SpriteRenderer>().getColor().a;
            out << YAML::EndSeq;

            out << YAML::Key << "Texture";
            out << YAML::Value << YAML::BeginMap;

            out << YAML::Key << "FilePath";
            out << YAML::Value << gameObject->getComponent<SpriteRenderer>().getTexture()->getFilePath();
            out << YAML::Key << "isTextureAtlas";
            out << YAML::Value << gameObject->getComponent<SpriteRenderer>().getTexture()->isTextureAtlas();
            out << YAML::Key << "TextureUnit";
            out << YAML::Value << gameObject->getComponent<SpriteRenderer>().getTexture()->getTextureUnit();
            out << YAML::EndMap;
            out << YAML::EndMap;

            out << YAML::EndMap;
            count++;
        }

        out << YAML::EndMap;
    }

    out << YAML::EndMap;
    std::ofstream file("../game/scene.yaml");
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
        std::cout << "gameObject is a map: " << it->second.IsMap() << std::endl;
        for (auto gameObject : it->second)
        {
            unsigned int width = gameObject.second["Width"].as<unsigned int>();
            unsigned int height = gameObject.second["Height"].as<unsigned int>();

            std::shared_ptr<GameObject> newGameObject = m_activeScene->addGameObject(width, height);
            // Deserialize transform component
            newGameObject->addComponent<Transform>(glm::vec3(0.0f, 0.0f, 0.0f));
            newGameObject->getComponent<Transform>().setPosition(
                gameObject.second["Transform"]["Position"][0].as<float>(),
                gameObject.second["Transform"]["Position"][1].as<float>(),
                gameObject.second["Transform"]["Position"][2].as<float>());
            newGameObject->getComponent<Transform>().setRotation(
                gameObject.second["Transform"]["Rotation"][0].as<float>(),
                gameObject.second["Transform"]["Rotation"][1].as<float>(),
                gameObject.second["Transform"]["Rotation"][2].as<float>());
            newGameObject->getComponent<Transform>().setScale(
                gameObject.second["Transform"]["Scale"][0].as<float>(),
                gameObject.second["Transform"]["Scale"][1].as<float>(),
                gameObject.second["Transform"]["Scale"][2].as<float>());

            // Deserialize sprite renderer component
            // First create a texture
            std::shared_ptr<Texture>
                texture = std::make_shared<Texture>(
                    gameObject.second["SpriteRenderer"]["Texture"]["FilePath"].as<std::string>().c_str(),
                    gameObject.second["SpriteRenderer"]["Texture"]["TextureUnit"]
                        .as<int>(),
                    gameObject.second["SpriteRenderer"]["Texture"]["isTextureAtlas"].as<bool>());
            texture->bind();

            newGameObject->addComponent<SpriteRenderer>(
                texture,
                gameObject.second["SpriteRenderer"]["spriteWidth"].as<unsigned int>(),
                gameObject.second["SpriteRenderer"]["spriteHeight"].as<unsigned int>(),
                gameObject.second["SpriteRenderer"]["xIndex"].as<unsigned int>(),
                gameObject.second["SpriteRenderer"]["yIndex"].as<unsigned int>());
        }
    }
    std::cout << "Deserialized scene from scene.yaml" << std::endl;
}