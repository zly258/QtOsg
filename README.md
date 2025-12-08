# QtOSG 3D模型浏览器

一个基于Qt和OpenSceneGraph(OSG)的轻量级3D模型浏览器，支持多种3D文件格式的查看和操作。

## 功能特性

- 🎯 **多格式支持**：支持LMB、GLTF/GLB等3D模型格式
- 🖥️ **现代化界面**：基于Qt的现代化用户界面
- 🌳 **场景结构树**：可视化显示3D场景的层级结构
- 🔍 **交互式操作**：支持模型旋转、缩放、平移等操作
- 📊 **性能监控**：实时显示内存使用情况
- ⚙️ **灵活设置**：支持坐标系统和缩放因子的配置

## 项目结构

```
QtOsg/
├── CMakeLists.txt          # 主CMake配置文件
├── README.md               # 项目说明文档
├── include/                # 头文件目录
│   ├── io/                 # 输入输出相关头文件
│   ├── utils/              # 工具类头文件
│   └── view/               # 界面相关头文件
├── src/                    # 源代码目录
│   ├── io/                 # 输入输出实现
│   ├── utils/              # 工具类实现
│   └── view/               # 界面实现
├── plugins/                # OSG插件目录
│   ├── osgdb_gltf/         # GLTF格式插件
│   └── osgdb_lmb/          # LMB格式插件
├── resources/              # 资源文件
│   ├── icons/              # 图标资源
│   └── i18n/               # 国际化资源
└── third-party/            # 第三方库
    └── tinygltf/           # GLTF解析库
```

## 构建要求

### 系统要求
- Windows 10/11 或 Linux
- CMake 3.14+
- C++17 兼容编译器

### 依赖库
- Qt 5.14.2+
- OpenSceneGraph 3.6.5+

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
   cmake ..
   ```

3. **编译项目**
   ```bash
   cmake --build . --config Release
   ```

## 使用说明

1. **启动应用**：运行生成的可执行文件
2. **打开模型**：通过"文件"菜单或快捷键打开3D模型文件
3. **视图操作**：
   - 鼠标左键：旋转视图
   - 鼠标右键：平移视图  
   - 鼠标滚轮：缩放视图
4. **场景管理**：通过左侧结构树查看和选择场景对象

## 支持的格式

- **LMB格式**：专有3D模型格式
- **GLTF/GLB**：标准3D传输格式

## 开发说明

### 代码规范
- 使用C++17标准
- 遵循Qt和OSG的命名规范
- 头文件使用#pragma once
- 类和方法使用中文注释说明

### 插件开发
项目支持OSG插件扩展，可在`plugins/`目录下添加新的文件格式支持。

## 许可证

本项目基于MIT许可证开源。

## 贡献

欢迎提交Issue和Pull Request来改进项目。