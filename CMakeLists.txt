cmake_minimum_required(VERSION 3.26.4)

project(myGame)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/engine)

add_executable(${PROJECT_NAME}
    main.cpp

    # game files
    game/Game.cpp
    game/systems/EnemySystem.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/
)

target_link_libraries(${PROJECT_NAME}
    engine
)

add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/assets
                $<TARGET_FILE_DIR:myGame>/assets
        COMMENT "Copying assets to build directory...")

add_dependencies(myGame copy_assets)