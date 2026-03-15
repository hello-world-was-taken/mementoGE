cmake_minimum_required(VERSION 3.26.4)

project(engine)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_subdirectory(engine/lib/glfw)
add_subdirectory(engine/lib/yaml-cpp)
add_subdirectory(engine/lib/box2d)
add_subdirectory(engine/lib/openal-soft)
add_subdirectory(engine/opengl)


add_library(${PROJECT_NAME} STATIC
    # core
    engine/core/components/EntityInfo.cpp
    engine/core/components/Transform.cpp
    engine/core/components/Sprite.cpp
    engine/core/components/RigidBody2D.cpp
    engine/core/components/BoxCollider2D.cpp
    engine/core/components/EnemyState.cpp
    engine/core/components/Patrol.cpp
    engine/core/components/Sensor2D.cpp
    engine/core/components/RenderLayer.cpp
    engine/core/components/PostProcessSettings.cpp
    engine/core/components/Camera.cpp
    engine/core/components/Animator.cpp
    engine/core/components/Text.cpp
    engine/core/components/TextAnchor.cpp
    engine/core/components/ParticleEmitter.cpp
    engine/core/components/AudioSource.cpp

    engine/core/Window.cpp
    engine/core/Camera.cpp  # TODO: remove. we have it as a component now
    engine/core/SceneCamera.cpp
    engine/core/GameObject.cpp
    engine/core/SceneManager.cpp
    engine/core/Scene.cpp
    engine/core/ImGuiWrapper.cpp
    engine/core/SpriteSheet.cpp
    engine/core/MouseListener.cpp
    engine/core/EventHandler.cpp
    engine/core/GlResourceManager.cpp
    engine/core/AnimationMap.cpp
    engine/core/AssetManager.cpp
    engine/core/AnimationPlayer.cpp
    engine/core/SystemRegistry.cpp
    engine/core/ParticleSystem.cpp
    engine/core/TextLayoutSystem.cpp
    engine/core/AudioSystem.cpp
    engine/core/AudioResourceManager.cpp
    engine/core/AudioPlayer.cpp

    # editor
    engine/editor/EditorLayer.cpp
    engine/editor/EditorContext.cpp
    engine/editor/ScenePanel.cpp
    engine/editor/SceneHierarchyPanel.cpp
    engine/editor/PropertiesPanel.cpp
    engine/editor/TexturePanel.cpp
    engine/editor/EditorMouseController.cpp
    engine/editor/EditorCamera.cpp
    engine/editor/SceneHistory.cpp
        engine/editor/AssetsPanel.cpp
        engine/editor/SpriteSheetEditorPanel.cpp

    # renderer
    engine/renderer/Renderer2D.cpp
    engine/renderer/RenderBatch.cpp
    engine/renderer/SpriteRenderer.cpp
    engine/renderer/GridRenderer.cpp
    engine/renderer/PhysicsRenderer.cpp
    engine/renderer/SelectionRenderer.cpp
    engine/renderer/CameraBoundsRenderer.cpp

    # physics
    engine/physics/Physics2D.cpp

    # utils
    engine/util/log_error.cpp
    engine/util/Time.cpp

    # third party
    engine/lib/stb_image/stb_image.cpp
    engine/lib/imgui/imgui.cpp

    engine/lib/imgui/imgui_draw.cpp
    engine/core/ComponentRegistry.cpp
    engine/lib/imgui/imgui_widgets.cpp
    engine/lib/imgui/imgui_tables.cpp
    engine/lib/imgui/backends/imgui_impl_glfw.cpp

    engine/lib/imgui/backends/imgui_impl_opengl3.cpp
    engine/lib/ImguiFileDialog/ImGuiFileDialog.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC engine/
    PUBLIC engine/lib
    PUBLIC engine/lib/glm
    PUBLIC engine/lib/imgui
    PUBLIC engine/lib/openal-soft/include
    PRIVATE engine/lib/ImguiFileDialog
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC glfw yaml-cpp box2d OpenAL
    PUBLIC OpenGL::GL
    PUBLIC GLEW::GLEW
    PRIVATE opengl
)

if(EDITOR_BUILD)
    target_compile_definitions(${PROJECT_NAME} PRIVATE EDITOR_BUILD)
endif()

# memento assets dir contains fonts, default shaders and so on
set(MEMENTO_ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/")
target_compile_definitions(${PROJECT_NAME} PRIVATE MEMENTO_ASSET_DIR="${MEMENTO_ASSET_DIR}")

# should be passed from the game
target_compile_definitions(${PROJECT_NAME} PUBLIC GAME_ASSETS_DIR="${GAME_ASSETS_DIR}")
