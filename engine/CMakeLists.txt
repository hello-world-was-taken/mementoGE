cmake_minimum_required(VERSION 3.26.4)

project(engine)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/yaml-cpp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/box2d)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/opengl)

add_library(${PROJECT_NAME} STATIC
    # core
    core/components/Transform.cpp
    core/components/Sprite.cpp
    core/components/RigidBody2D.cpp
    core/components/BoxCollider2D.cpp
    core/components/EnemyState.cpp
    core/components/Patrol.cpp
    # core/components/Animation.cpp

    core/Window.cpp
    core/Camera.cpp
    core/SceneCamera.cpp
    core/GameObject.cpp
    core/SceneManager.cpp
    core/Scene.cpp
    core/ImGuiWrapper.cpp
    core/SpriteSheet.cpp
    core/MouseListener.cpp
    core/Event.cpp
    core/EventHandler.cpp
    core/GlResourceManager.cpp
    core/AnimationMap.cpp
    core/AssetManager.cpp
    core/AnimationPlayer.cpp
    core/Animator.cpp
    core/SystemRegistry.cpp

    # editor
    editor/EditorLayer.cpp
    editor/ScenePanel.cpp
    editor/SceneHierarchyPanel.cpp
    editor/PropertiesPanel.cpp
    editor/TexturePanel.cpp
    editor/EditorMouseController.cpp
    editor/EditorCamera.cpp

    # renderer
    renderer/RenderBatch.cpp
    renderer/SpriteRenderer.cpp
    renderer/GridRenderer.cpp
    renderer/PhysicsRenderer.cpp

    # physics
    physics/Physics2D.cpp

    # utils
    util/log_error.cpp
    util/Time.cpp

    # third party
    lib/stb_image/stb_image.cpp
    lib/imgui/imgui.cpp lib/imgui/imgui_draw.cpp lib/imgui/imgui_widgets.cpp
    lib/imgui/imgui_tables.cpp lib/imgui/backends/imgui_impl_glfw.cpp
    lib/imgui/backends/imgui_impl_opengl3.cpp
    lib/ImguiFileDialog/ImGuiFileDialog.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/glm
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/ImguiFileDialog
)

target_link_libraries(${PROJECT_NAME}
    glfw
    yaml-cpp
    box2d
    GLEW::GLEW  # Should be installed to the the system.
    ${OPENGL_LIBRARIES}
    opengl # wrapper of opengl calls
)

# comment out for release builds
target_compile_definitions(${PROJECT_NAME} PRIVATE EDITOR_BUILD)

# Configure for MacOs
if(APPLE)
    find_library(OpenGL_FRAMEWORK OpenGL)
    find_library(Cocoa_FRAMEWORK Cocoa)
    find_library(IOKit_FRAMEWORK IOKit)
    find_library(CoreVideo_FRAMEWORK CoreVideo)

    target_link_libraries(${PROJECT_NAME}
        ${OpenGL_FRAMEWORK}
        ${Cocoa_FRAMEWORK}
        ${IOKit_FRAMEWORK}
        ${CoreVideo_FRAMEWORK}
    )
endif()
