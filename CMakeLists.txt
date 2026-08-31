cmake_minimum_required(VERSION 3.14)
project(bydxxq LANGUAGES CXX)

# std=c++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_custom_target(link_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${CMAKE_SOURCE_DIR}/compile_commands.json"
    COMMENT "Syncing compile_commands.json to root for clangd"
)

# manage multi projects
add_subdirectory(test-qt)
add_subdirectory(test-qt-another)
