#include <yaml-cpp/yaml.h>
#include <imgui.h>
#include <filesystem>
#include <fstream>

#include "core/SceneManager.h"
#include "core/ImGuiWrapper.h"
#include "core/Resource.h"
#include "core/Sprite.h"
#include "core/SpriteSheet.h"
#include "core/GameObject.h"

SceneManager::SceneManager(Window *window, const EventHandler &eventHandler)
    : m_window{window},
      mEventHandler{eventHandler}
{
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
    m_activeScene->start();
}

void SceneManager::update(float deltaTime)
{
    if (m_activeScene == nullptr)
    {
        std::cout << "No active m_scene found" << std::endl;
        return;
    }
    m_eventHandlerFunction(*m_window, this, mEventHandler);

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
        this->renderTextureResourcesImGui();
        mMouseActionController.Update(getActiveScene()->getCamera());

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

GameObject &SceneManager::getActiveGameObject() const
{
    return m_activeScene->getActiveGameObject();
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
        m_activeScene = &(it->second);
        m_activeScene->start();
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

void SceneManager::addScene(const char *sceneName, Scene &&scene)
{
    m_scenes.insert({sceneName, std::move(scene)});
    // TODO: shouldn't the active scene be set to the latest scene added?
    if (m_activeScene == nullptr)
    {
        m_activeScene = &(m_scenes.find(sceneName)->second);
    }
}

void SceneManager::removeScene(const char *sceneName)
{
    m_scenes.erase(sceneName);
}

void SceneManager::renderTextureResourcesImGui()
{
    // TODO: use this as a dummy sprite to render the texture resources change it later on.
    SpriteSheet spriteSheet = SpriteSheet("../assets/texture/spritesheet_retina.png", true, 128, 0);
    ImGui::Text("Sprites");
    std::shared_ptr<Texture> spriteSheetTexture = spriteSheet.getTexture();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();

    float windowX2 = windowPos.x + windowSize.x;
    int id = 0;
    for (Sprite sprite : spriteSheet.getSprites())
    {
        float spriteWidth = spriteSheet.getSubTextureSize();
        float spriteHeight = spriteSheet.getSubTextureSize();
        std::vector<glm::vec2> textureCoordinates = sprite.getTextureCoordinates();
        ImTextureID texId = (ImTextureID)(uintptr_t)spriteSheetTexture->getId();

        // TODO: Add sprite IDs and use those to identify which sprite was clicked
        ImGui::PushID(id);
        if (ImGui::ImageButton(
                "",
                texId,
                ImVec2(spriteWidth, spriteHeight),
                ImVec2(textureCoordinates[0].x,
                       textureCoordinates[0].y), // uv0 = top-left
                ImVec2(textureCoordinates[2].x,
                       textureCoordinates[2].y), // uv1 = bottom-right
                ImVec4(0.0f, 0.0f, 0.0f, 1.0f),
                ImVec4(1.0f, 1.0f, 1.0f, 1.0f)))
        {
            Scene *activeScene = getActiveScene();
            activeScene->addGameObject(32, 32, "_new");
            activeScene->getActiveGameObject().addComponent<Sprite>(
                "../assets/texture/spritesheet_retina.png",
                true,
                sprite.getTextureCoordinates());
            mMouseActionController.SetActiveObject(activeScene->getActiveGameObject());
        }
        ImGui::PopID();

        ImVec2 lastSpritePosition = ImGui::GetItemRectMax();
        float lastSpriteX2 = lastSpritePosition.x;
        float nextButtonX2 = lastSpriteX2 + spriteWidth;
        if (id + 1 < spriteSheet.getSprites().size() && nextButtonX2 < windowX2)
        {
            ImGui::SameLine();
        }
        id++;
    }
}

Scene *SceneManager::getActiveScene()
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
        out << YAML::Key << "Game Objects";
        out << YAML::Value << YAML::BeginMap;

        for (GameObject &gameObject : scene.getGameObjects())
        {
            gameObject.serialize(out);
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

    // TODO: We are assuming we'll always have a single scene. fix.
    YAML::Node serializedScene = YAML::LoadFile("../game/scene.yaml");
    Scene scene = Scene{std::move(serializedScene)};

    m_scenes.insert_or_assign("default_scene", std::move(scene));
    // TODO: map.find() returns iterator end if the value is not present. Fix it.
    // TODO: active scene should be handled better.
    m_activeScene = &(m_scenes.find("default_scene")->second);

    std::cout << "Deserialized scene from scene.yaml" << std::endl;
}