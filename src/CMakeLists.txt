set(SOURCES
    view/OSGWidget.cpp
    view/MainWindow.cpp
    main.cpp
    ../resources/resources.qrc
)

set(HEADERS
    view/OSGWidget.h
    view/MainWindow.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL Qt5::Svg
    PRIVATE ${OPENSCENEGRAPH_LIBRARIES}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Psapi)
endif()
