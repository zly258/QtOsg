cmake_minimum_required(VERSION 3.14)

project(LMBModelViewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

# 设置并行编译
if(MSVC)
    add_compile_options(/MP)
    add_compile_options(/bigobj) 
endif()

# 全局启用 UTF-8 源码/执行字符集（MSVC）
add_compile_options(
    $<$<C_COMPILER_ID:MSVC>:/utf-8>
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

# 设置第三方路径
set(QT_DIR "D:/tools/QT/5.14.2/msvc2017_64" CACHE PATH "Qt5 root path")
set(OSG_DIR "D:/tools/OSG/OpenSceneGraph-3.6.5-debug" CACHE PATH "OSG root path")

# 查找Qt包
set(CMAKE_PREFIX_PATH ${QT_DIR})
find_package(Qt5 COMPONENTS Core Gui Widgets OpenGL Svg REQUIRED)

# 查找 OSG 包
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${OSG_DIR})
find_package(OpenSceneGraph REQUIRED COMPONENTS 
    osg 
    osgDB 
    osgGA 
    osgUtil 
    osgViewer 
    osgText
    osgAnimation
)

# 包含头文件目录（仅第三方）
include_directories(
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
)



# 定义必要的宏
add_definitions(-DUSE_OSG)

# 添加子目录
add_subdirectory(src)
add_subdirectory(plugins)
