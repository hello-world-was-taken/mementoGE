cmake_minimum_required(VERSION 3.26.4)

project(mementoGE)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)

# add external libraries (glfw) as a submodule. We have to build glew manually, because it doesn't have CMakeLists.text. Therefore, we need to build it manually by running the Makefile it contains.
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/glfw)

# since the imported libraries (glfw, glew) don't change, I don't think we need to add them here
add_executable(${PROJECT_NAME} 
    main.cpp 
    
    # core
    engine/core/Window.cpp
    engine/core/Transform.cpp
    engine/core/Camera.cpp
    engine/core/GameObject.cpp 
    engine/core/SceneManager.cpp
    engine/core/Scene.cpp
    engine/core/ImGuiWrapper.cpp

    # opengl
    engine/opengl/Texture.cpp
    engine/opengl/IndexBuffer.cpp
    engine/opengl/VertexBuffer.cpp
    engine/opengl/Shader.cpp

    # renderer
    engine/renderer/RenderBatch.cpp
    engine/renderer/SpriteRenderer.cpp

    # utils
    util/log_error.cpp
    util/Time.cpp
    
    # game files
    game/Application.cpp
    game/MouseListener.cpp
    game/KeyListener.cpp 
    
    # third party
    lib/stb_image/stb_image.cpp
    lib/imgui/imgui.cpp lib/imgui/imgui_draw.cpp lib/imgui/imgui_widgets.cpp
    lib/imgui/imgui_tables.cpp lib/imgui/backends/imgui_impl_glfw.cpp 
    lib/imgui/backends/imgui_impl_opengl3.cpp
)

# get a hold of the frameworks we'll need. These are already installed and come with OSX.
find_library(OpenGL_FRAMEWORK OpenGL)
find_library(Cocoa_FRAMEWORK Cocoa)
find_library(IOKit_FRAMEWORK IOKit)
find_library(CoreVideo_FRAMEWORK CoreVideo)
find_package(GLEW REQUIRED)

# include directories for our libs (glfw, glew)
target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_SOURCE_DIR}/
    PUBLIC ${CMAKE_SOURCE_DIR}/lib
    PUBLIC ${CMAKE_SOURCE_DIR}/lib/glm
    PUBLIC ${CMAKE_SOURCE_DIR}/lib/imgui
    PUBLIC ${CMAKE_SOURCE_DIR}/lib/glfw/include
    PUBLIC ${CMAKE_SOURCE_DIR}/lib/yaml-cpp/include
)

# linking directories for our libs (glfw, glew)
target_link_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_SOURCE_DIR}/out/lib/glfw/src
)

# linking the actual binaries for our libs (glfw, glew)
target_link_libraries(${PROJECT_NAME}
    # 'glfw3' won't work. It should be just 'glfw'
    glfw
    GLEW::glew  # For this one, I installed to the the system.
    ${OpenGL_FRAMEWORK}
    ${Cocoa_FRAMEWORK}
    ${IOKit_FRAMEWORK}
    ${CoreVideo_FRAMEWORK}
)