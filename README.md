# QtOSG 3D模型浏览器

一个基于Qt和OpenSceneGraph(OSG)的轻量级3D模型浏览器，支持多种3D文件格式的查看和操作。

![QtOSG](https://img.shields.io/badge/Qt-5.14.2+-green.svg)
![OSG](https://img.shields.io/badge/OSG-3.6.5+-blue.svg)
![CMake](https://img.shields.io/badge/CMake-3.14+-blue.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

## 功能特性

- 🎯 **多格式支持**：支持LMB、GLTF/GLB、OSG、OSGT、IVE、OBJ等3D模型格式
- 🖥️ **现代化界面**：基于Qt的现代化用户界面，支持中文本地化
- 🌳 **场景结构树**：可视化显示3D场景的层级结构，支持节点选择
- 🔍 **交互式操作**：支持模型旋转、缩放、平移等操作
- 📊 **性能监控**：实时显示帧率和内存使用情况
- ⚙️ **灵活设置**：支持正交/透视视图切换，多种标准视图
- 🎨 **渲染控制**：支持线框模式、背面剔除、光照开关
- 📝 **属性查看**：显示选中节点的详细属性信息

## 项目结构

```
QtOsg/
├── CMakeLists.txt          # 主CMake配置文件
├── README.md               # 项目说明文档
├── src/                    # 源代码目录
│   ├── CMakeLists.txt      # 源码CMake配置
│   ├── main.cpp            # 应用程序入口
│   └── view/               # 界面相关代码
│       ├── MainWindow.h/cpp    # 主窗口实现
│       └── OSGWidget.h/cpp     # OSG渲染组件
├── plugins/                # OSG插件目录
│   ├── CMakeLists.txt      # 插件CMake配置
│   ├── osgdb_gltf/         # GLTF格式插件
│   │   ├── ReaderWriterGLTF.h/cpp
│   │   ├── GltfParser.h/cpp
│   │   └── CMakeLists.txt
│   └── osgdb_lmb/          # LMB格式插件
│       ├── ReaderWriterLMB.h/cpp
│       ├── LmbParser.h/cpp
│       └── CMakeLists.txt
├── resources/              # 资源文件
│   ├── icons/              # 图标资源
│   │   ├── app_icon.svg
│   │   ├── geode.svg
│   │   ├── geometry_multiple.svg
│   │   ├── geometry_single.svg
│   │   └── scene_root.svg
│   ├── qt_zh_CN.qm         # Qt中文翻译
│   ├── qtbase_zh_CN.qm     # Qt基础中文翻译
│   └── resources.qrc       # 资源配置文件
└── third-party/            # 第三方库
    ├── glfw/               # GLFW库
    ├── imgui/              # Dear ImGui库
    └── tinygltf/           # GLTF解析库
```

## 构建要求

### 系统要求
- Windows 10/11 或 Linux
- CMake 3.14+
- C++17 兼容编译器 (推荐MSVC 2017+)

### 依赖库
- Qt 5.14.2+ (Core, Gui, Widgets, OpenGL, Svg组件)
- OpenSceneGraph 3.6.5+ (osg, osgDB, osgGA, osgUtil, osgViewer, osgText, osgAnimation)

## 构建步骤

1. **配置环境变量**
   ```bash
   # 设置Qt路径
   set QT_DIR=D:/tools/QT/5.14.2/msvc2017_64
   
   # 设置OSG路径  
   set OSG_DIR=D:/tools/OSG/OpenSceneGraph-3.6.5-debug
   ```

2. **生成构建文件**
   ```bash
   mkdir build
   cd build
   cmake .. -DQT_DIR="D:/tools/QT/5.14.2/msvc2017_64" -DOSG_DIR="D:/tools/OSG/OpenSceneGraph-3.6.5-debug"
   ```

3. **编译项目**
   ```bash
   cmake --build . --config Release
   ```

4. **运行程序**
   ```bash
   # Windows
   cd build/Release
   LMBModelViewer.exe
   
   # Linux
   cd build
   ./LMBModelViewer
   ```

## 使用说明

### 基本操作

1. **启动应用**：运行生成的可执行文件
2. **打开模型**：通过"文件"菜单或快捷键打开3D模型文件
3. **视图操作**：
   - 鼠标左键：旋转视图
   - 鼠标右键：平移视图  
   - 鼠标滚轮：缩放视图
   - 鼠标中键：平移视图（替代右键）
4. **场景管理**：通过左侧结构树查看和选择场景对象
5. **属性查看**：选中节点后，右侧面板显示详细属性

### 高级功能

1. **视图切换**：
   - 正交视图/透视视图切换
   - 六种标准视图：前视、后视、左视、右视、顶视、底视

2. **渲染控制**：
   - W键：切换线框模式
   - B键：切换背面剔除
   - L键：切换光照
   - S键：显示统计信息

3. **节点选择**：
   - 左键点击模型选择节点
   - 选中节点高亮显示（红色）
   - 属性面板显示节点详细信息

## 支持的格式

- **LMB格式**：专有3D模型格式
- **GLTF/GLB**：标准3D传输格式
- **OSG/OSGT**：OpenSceneGraph原生格式
- **IVE**：OpenSceneGraph二进制格式
- **OBJ**：Wavefront 3D对象格式

## 技术架构

### 核心组件

1. **MainWindow**：主窗口类，负责整体UI布局和菜单管理
   - 集成OSGWidget作为中央渲染窗口
   - 左侧停靠窗口显示场景结构树
   - 右侧停靠窗口显示节点属性
   - 底部状态栏显示性能统计

2. **OSGWidget**：OpenGL渲染组件，基于QOpenGLWidget
   - 集成OSG渲染引擎
   - 处理鼠标和键盘交互
   - 实现模型加载和场景管理
   - 提供节点选择和高亮功能

3. **插件系统**：支持多种3D文件格式
   - ReaderWriterLMB：LMB格式插件
   - ReaderWriterGLTF：GLTF/GLB格式插件

### 渲染特性

- 多重采样抗锯齿（MSAA 4x）
- 正交和透视投影支持
- 自适应场景缩放
- 节点拾取和高亮
- 实时性能监控

## 开发说明

### 代码规范
- 使用C++17标准
- 遵循Qt和OSG的命名规范
- 头文件使用#pragma once
- 类和方法使用中文注释说明

### 插件开发
项目支持OSG插件扩展，可在`plugins/`目录下添加新的文件格式支持：
1. 创建新的插件目录（如osgdb_xxx）
2. 实现ReaderWriter接口
3. 在plugins/CMakeLists.txt中添加子目录
4. 在main.cpp中注册插件

### 自定义构建
可以通过修改CMakeLists.txt中的路径变量来适应不同的开发环境：
```cmake
set(QT_DIR "你的Qt路径" CACHE PATH "Qt5 root path")
set(OSG_DIR "你的OSG路径" CACHE PATH "OSG root path")
```

## 许可证

本项目基于MIT许可证开源。详见[LICENSE](LICENSE)文件。

## 贡献

欢迎提交Issue和Pull Request来改进项目。

### 贡献指南
1. Fork本项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建Pull Request

## 更新日志

### v1.0.0 (当前版本)
- 初始版本发布
- 支持LMB和GLTF/GLB格式
- 基本的3D查看功能
- 场景树和属性面板
- 多种视图模式

## 联系方式

如有问题或建议，请通过以下方式联系：
- 提交Issue：[GitHub Issues](https://github.com/zly258/QtOsg/issues)
- 邮箱：zly258@example.com

## 致谢

感谢以下开源项目：
- [Qt](https://www.qt.io/) - 跨平台应用框架
- [OpenSceneGraph](http://www.openscenegraph.org/) - 3D图形工具包
- [Dear ImGui](https://github.com/ocornut/imgui) - 即时模式GUI库
- [tinygltf](https://github.com/syoyo/tinygltf) - GLTF解析库