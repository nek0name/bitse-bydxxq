find_package(Qt6 REQUIRED COMPONENTS Gui)
if(NOT ANDROID)
  find_package(Qt6 REQUIRED COMPONENTS DBus)
endif()
add_library(charging-client STATIC ApiClient.cpp ApiClient.h Fonts.cpp Fonts.h Appearance.cpp Appearance.h fonts.qrc)
set(CHARGING_DEFAULT_SERVER_URL "https://www.u910784.nyat.app:40004" CACHE STRING
  "Default service URL for clients; CHARGING_SERVER_URL overrides it at runtime")
target_compile_definitions(charging-client PRIVATE
  CHARGING_DEFAULT_SERVER_URL="${CHARGING_DEFAULT_SERVER_URL}")
target_include_directories(charging-client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(charging-client PUBLIC Qt6::Core Qt6::Network Qt6::Gui)
if(NOT ANDROID)
  target_link_libraries(charging-client PUBLIC Qt6::DBus)
endif()
