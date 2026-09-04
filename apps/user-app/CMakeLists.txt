qt_add_executable(charging-user
    main.cpp
    UserMainWindow.cpp
    UserMainWindow.h
    resources.qrc
)
target_link_libraries(charging-user PRIVATE Qt6::Widgets)
set_target_properties(charging-user PROPERTIES WIN32_EXECUTABLE TRUE)
