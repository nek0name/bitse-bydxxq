qt_add_executable(charging-admin main.cpp AdminMainWindow.cpp AdminMainWindow.h)
target_link_libraries(charging-admin PRIVATE Qt6::Widgets)
set_target_properties(charging-admin PROPERTIES WIN32_EXECUTABLE TRUE)
