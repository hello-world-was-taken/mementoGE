cmake_minimum_required(VERSION 3.26.4)

project(mementoGE)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)

# Subdirectories are libraries we build along with our project.
# If they were found in the system, we wouldn't need to add them here.
# Additionally, if they were header only, we wouldn't need to add them here
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/glfw)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/yaml-cpp)

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
    engine/core/Resource.cpp
    engine/core/Sprite.cpp
    engine/core/SpriteSheet.cpp

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

# include directories for our libs (glfw, glew)
target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_SOURCE_DIR}/
    PUBLIC ${CMAKE_SOURCE_DIR}/lib
    PUBLIC ${CMAKE_SOURCE_DIR}/lib/glm
    PUBLIC ${CMAKE_SOURCE_DIR}/lib/imgui
    PUBLIC ${CMAKE_SOURCE_DIR}/lib/glfw/include
    PUBLIC ${CMAKE_SOURCE_DIR}/lib/yaml-cpp/include
)

# In addition to the system libraries, for the ones we built along with our project,
# we need to add the directories where the binaries are.
# For header only libraries, we wouldn't need to do this.
# This adds additional path to the linker. It doesn't link the libraries.
target_link_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_SOURCE_DIR}/out/lib/glfw/src
    PUBLIC ${CMAKE_SOURCE_DIR}/out/lib/yaml-cpp
)

# link the libraries we built as well as the system libraries.
# Here, we are actually linking the libraries.
target_link_libraries(${PROJECT_NAME}
    # 'glfw3' won't work. It should be just 'glfw'
    glfw
    yaml-cpp
    GLEW::glew  # For this one, I installed it to the the system.
)

# Configure for Unix/Linux
if(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)

    target_link_libraries(${PROJECT_NAME}
        ${OPENGL_LIBRARIES}
    )
endif()

# Configure for Windows. Haven't run it on windows yet.
if(WIN32)
    # Specific settings for Windows
    target_link_libraries(${PROJECT_NAME}
        opengl32 # Link against OpenGL on Windows
        # If we want to run on windows, we may need to add other windown speicific libraries
    )
endif()

# Configure for MacOs
if(APPLE)
    find_library(OpenGL_FRAMEWORK OpenGL)
    find_library(Cocoa_FRAMEWORK Cocoa)
    find_library(IOKit_FRAMEWORK IOKit)
    find_library(CoreVideo_FRAMEWORK CoreVideo)
    find_package(GLEW REQUIRED)

    target_link_libraries(${PROJECT_NAME}
        ${OpenGL_FRAMEWORK}
        ${Cocoa_FRAMEWORK}
        ${IOKit_FRAMEWORK}
        ${CoreVideo_FRAMEWORK}
    )
endif()