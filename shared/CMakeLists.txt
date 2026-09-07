find_package(Qt6 REQUIRED COMPONENTS Gui DBus)
add_library(charging-client STATIC ApiClient.cpp ApiClient.h Fonts.cpp Fonts.h Appearance.cpp Appearance.h fonts.qrc)
target_include_directories(charging-client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(charging-client PUBLIC Qt6::Core Qt6::Network Qt6::Gui Qt6::DBus)
