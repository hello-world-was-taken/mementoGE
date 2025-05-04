cmake_minimum_required(VERSION 3.26.4)

project(myGame)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(${CMAKE_SOURCE_DIR}/engine)

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

target_link_libraries(${PROJECT_NAME}
    engine
)