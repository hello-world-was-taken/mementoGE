cmake_minimum_required(VERSION 3.26.4)

project(opengl_playground)

# since the imported libraries (glfw, glew) don't change, I don't think we need to add them here
add_executable(${PROJECT_NAME} main.cpp)

# add external libraries (glfw) as a submodule. We have to build glew manually, because it doesn't have CMakeLists.text. Therefore, we need to build it manually by running the Makefile it contains.
add_subdirectory(
    ${CMAKE_SOURCE_DIR}/lib/glfw
)

# get a hold of the frameworks we'll need. These are already installed and come with OSX.
find_library(OpenGL_FRAMEWORK OpenGL)
find_library(Cocoa_FRAMEWORK Cocoa)
find_library(IOKit_FRAMEWORK IOKit)
find_library(CoreVideo_FRAMEWORK CoreVideo)

# include directories for our libs (glfw, glew)
target_include_directories(${PROJECT_NAME} 
    PUBLIC ${CMAKE_SOURCE_DIR}/lib/glfw/include
    PUBLIC ${CMAKE_SOURCE_DIR}/lib/glew/include
)

# linking directories for our libs (glfw, glew)
target_link_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_SOURCE_DIR}/lib/glfw/src
    PUBLIC ${CMAKE_SOURCE_DIR}/lib/glew/lib
)

# linking the actual binaries for our libs (glfw, glew)
target_link_libraries(${PROJECT_NAME}
    glfw3
    GLEW
    ${OpenGL_FRAMEWORK}
    ${Cocoa_FRAMEWORK}
    ${IOKit_FRAMEWORK}
    ${CoreVideo_FRAMEWORK}
)