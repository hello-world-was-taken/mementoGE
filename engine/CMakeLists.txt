cmake_minimum_required(VERSION 3.26.4)

project(engine)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)

# add opengl, glfw and yaml-cpp to build
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/yaml-cpp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/opengl)

# target - build the engine as a static library.
add_library(${PROJECT_NAME} STATIC
    # core
    core/Window.cpp
    core/Transform.cpp
    core/Camera.cpp
    core/GameObject.cpp
    core/SceneManager.cpp
    core/Scene.cpp
    core/ImGuiWrapper.cpp
    core/Resource.cpp
    core/Sprite.cpp
    core/SpriteSheet.cpp
    core/MouseListener.cpp
    core/Event.cpp
    core/EventHandler.cpp
    core/MouseActionController.cpp

    # renderer
    renderer/RenderBatch.cpp
    renderer/SpriteRenderer.cpp

    # utils
    util/log_error.cpp
    util/Time.cpp

    # third party
    lib/stb_image/stb_image.cpp
    lib/imgui/imgui.cpp lib/imgui/imgui_draw.cpp lib/imgui/imgui_widgets.cpp
    lib/imgui/imgui_tables.cpp lib/imgui/backends/imgui_impl_glfw.cpp
    lib/imgui/backends/imgui_impl_opengl3.cpp
)

# include directories for our libs (glfw, glew)
target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/glm
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw/include
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/yaml-cpp/include
)

# In addition to the system libraries, for the ones we built along with our project,
# we need to add the directories where the binaries are.
target_link_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_SOURCE_DIR}/out/lib/glfw/src
    PUBLIC ${CMAKE_SOURCE_DIR}/out/lib/yaml-cpp
    PUBLIC ${CMAKE_SOURCE_DIR}/out/opengl
)

target_link_libraries(${PROJECT_NAME}
    # 'glfw3' won't work. It should be just 'glfw'
    glfw
    yaml-cpp
    GLEW::glew  # For this one, I installed it to the the system.
    opengl # my wrapper of opengl calls
)









####################################################################
#      THIS IS COMMON TO ALL THE CMAKEFILES SHOULD BE EXTRACTED.
####################################################################

# For glew, I used homebrew to install it. So, I need to add the path to the include directory.
# I tried to add the if condition inside the target_include_directories above, but it didn't work.
if(APPLE)
    target_include_directories(${PROJECT_NAME}
        PUBLIC /opt/homebrew/Cellar/glew/2.2.0_1/include
    )
endif()

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