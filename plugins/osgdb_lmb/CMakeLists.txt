# OSG LMB Plugin CMakeLists.txt

# 设置插件名称
set(PLUGIN_NAME osgdb_lmb)

# 源文件
set(PLUGIN_SOURCES
    ReaderWriterLMB.cpp
    LmbParser.cpp
    ../PluginLogger.cpp
)

# 头文件
set(PLUGIN_HEADERS
    ReaderWriterLMB.h
    LmbParser.h
    ../PluginLogger.h
)

# 创建插件库
add_library(${PLUGIN_NAME} MODULE
    ${PLUGIN_SOURCES}
    ${PLUGIN_HEADERS}
)

# 设置目标属性
set_target_properties(${PLUGIN_NAME} PROPERTIES
    PREFIX ""  # OSG 插件不需要 lib 前缀
    FOLDER "OSG Plugins"
)

# 包含目录
target_include_directories(${PLUGIN_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(${PLUGIN_NAME}
    ${OPENSCENEGRAPH_LIBRARIES}
)

# 定义宏
target_compile_definitions(${PLUGIN_NAME} PRIVATE
    -DUSE_OSG
)

# 设置输出目录到 OSG 插件目录
if(WIN32)
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/osgPlugins-3.6.5"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/osgPlugins-3.6.5"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/osgPlugins-3.6.5"
    )
else()
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/osgPlugins-3.6.5"
    )
endif()

# 定义OSG安装目录
set(OSG_PLUGIN_DIR "D:/tools/OSG/OpenSceneGraph-3.6.5-debug/bin/osgPlugins-3.6.5")

# 添加自定义命令，编译后自动复制到OSG插件目录
if(WIN32)
    add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OSG_PLUGIN_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
            "$<TARGET_FILE:${PLUGIN_NAME}>" 
            "${OSG_PLUGIN_DIR}/"
        COMMENT "Copying ${PLUGIN_NAME} to OSG plugin directory"
    )
endif()

# 安装插件
install(TARGETS ${PLUGIN_NAME}
    RUNTIME DESTINATION osgPlugins-3.6.5
    LIBRARY DESTINATION osgPlugins-3.6.5
)