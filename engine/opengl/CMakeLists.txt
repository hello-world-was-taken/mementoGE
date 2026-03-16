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
    libglew_static
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib/glm
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib/yaml-cpp/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib/stb_image
)
