cmake_minimum_required(VERSION 3.16)
project(charging_platform LANGUAGES CXX)

# Qt 6 推荐使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

option(BUILD_USER_APP "Build the charging user application" ON)
option(BUILD_ADMIN_APP "Build the operations admin application" ON)

# for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_custom_target(link_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${CMAKE_SOURCE_DIR}/compile_commands.json"
    COMMENT "Syncing compile_commands.json to root for clangd"
)

add_custom_target(format
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/format-cpp.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Formatting all C/C++ sources"
)

add_custom_target(format-check
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/format-cpp.sh" --check
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Checking C/C++ source formatting"
)

if(BUILD_USER_APP)
    add_subdirectory(apps/user-app)
endif()

if(BUILD_ADMIN_APP)
    add_subdirectory(apps/admin-app)
endif()
