cmake_minimum_required(VERSION 3.26.4)

project(opengl)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)

#target
add_library(${PROJECT_NAME} STATIC
    # opengl
    Texture.cpp
    IndexBuffer.cpp
    VertexBuffer.cpp
    VertexArray.cpp
    Shader.cpp
)

target_link_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_SOURCE_DIR}/out/lib/glfw/src
    PUBLIC ${CMAKE_SOURCE_DIR}/out/lib/yaml-cpp
)

target_link_libraries(${PROJECT_NAME}
    glfw
    yaml-cpp
    GLEW::glew  # For this one, I installed it to the the system.
)

# what is the point of having a separate cmakefile if we are still
# have similar dependencies?
target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_SOURCE_DIR}/engine/lib
    PUBLIC ${CMAKE_SOURCE_DIR}/engine/lib/glm
    PUBLIC ${CMAKE_SOURCE_DIR}/engine/lib/glfw/include
    PUBLIC ${CMAKE_SOURCE_DIR}/engine/lib/yaml-cpp/include
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