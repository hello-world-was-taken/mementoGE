cmake_minimum_required(VERSION 3.26.4)

project(opengl)

set(CMAKE_CXX_STANDARD 17)

#target
add_library(${PROJECT_NAME} STATIC
    Texture.cpp
    IndexBuffer.cpp
    VertexBuffer.cpp
    VertexArray.cpp
    Shader.cpp
    FrameBuffer.cpp
)

target_link_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_SOURCE_DIR}/out/engine/lib/yaml-cpp
)

target_link_libraries(${PROJECT_NAME}
    yaml-cpp
    GLEW::glew
)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_SOURCE_DIR}/engine/lib
    PUBLIC ${CMAKE_SOURCE_DIR}/engine/lib/glm
    PUBLIC ${CMAKE_SOURCE_DIR}/engine/lib/yaml-cpp/include
)
