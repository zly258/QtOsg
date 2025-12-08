# OSG Plugins CMakeLists.txt

message(STATUS "Building OSG Plugins...")

# 检查是否找到了 OSG
if(NOT OPENSCENEGRAPH_FOUND)
    message(WARNING "OpenSceneGraph not found, skipping plugin build")
    return()
endif()

# 创建 osgPlugins 目录
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/osgPlugins-3.6.5)

# 添加插件子目录
add_subdirectory(osgdb_lmb)
add_subdirectory(osgdb_gltf)

message(STATUS "OSG Plugins will be built to: ${CMAKE_BINARY_DIR}/osgPlugins-3.6.5")