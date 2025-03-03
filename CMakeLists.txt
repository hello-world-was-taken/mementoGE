cmake_minimum_required(VERSION 3.26.4)

project(myGame)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)

# Subdirectories are libraries we build along with our project.
add_subdirectory(${CMAKE_SOURCE_DIR}/engine)

# since the imported libraries (glfw, glew) don't change, I don't think we need to add them here
add_executable(${PROJECT_NAME}
    main.cpp

    # game files
    game/Application.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_SOURCE_DIR}/
)

target_link_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_SOURCE_DIR}/engine
)

# link the libraries we built as well as the system libraries.
target_link_libraries(${PROJECT_NAME}
    engine
)