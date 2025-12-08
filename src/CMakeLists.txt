# 手动指定源文件（仅 OSG 构建）
set(ALL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/StringUtil.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/view/MainWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/SelectionUtil.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/view/OsgContainer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/view/FileSettingsDialog.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/SceneStructureParser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/io/ModelLoader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/io/ProgressManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/io/FileFilter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../resources/resources.qrc"
)

set(ALL_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/utils/StringUtil.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/view/MainWindow.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/utils/SelectionUtil.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/view/OsgContainer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/view/FileSettingsDialog.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/utils/SceneStructureParser.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/io/ModelLoader.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/io/ProgressManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/io/FileFilter.h"
)

# 创建可执行文件
add_executable(LMBModelViewer
    ${ALL_SOURCES}
    ${ALL_HEADERS}
)

# 链接库
target_link_libraries(LMBModelViewer
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::OpenGL
    Qt5::Svg
)

target_link_libraries(LMBModelViewer
    ${OPENSCENEGRAPH_LIBRARIES}
)

# 设置目标属性
set_target_properties(LMBModelViewer PROPERTIES
    WIN32_EXECUTABLE TRUE
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Windows 控制台设置
if(WIN32)
    set_target_properties(LMBModelViewer PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE /ignore:4099"
    )
endif()

# 复制OSG插件到可执行文件目录（重要！）
if(WIN32)
    # 获取OSG插件目录路径
    set(OSG_PLUGIN_DIR "${OSG_DIR}/bin/osgPlugins-3.6.5")
    
    # 创建插件目录
    add_custom_command(TARGET LMBModelViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:LMBModelViewer>/osgPlugins-3.6.5"
    )
    
    # 复制所有OSG插件文件
    add_custom_command(TARGET LMBModelViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory 
        "${OSG_PLUGIN_DIR}" 
        "$<TARGET_FILE_DIR:LMBModelViewer>/osgPlugins-3.6.5"
    )
    
    message(STATUS "OSG插件将复制到: $<TARGET_FILE_DIR:LMBModelViewer>/osgPlugins-3.6.5")
    message(STATUS "从源目录复制: ${OSG_PLUGIN_DIR}")
endif()