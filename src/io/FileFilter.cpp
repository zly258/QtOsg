#include "io/FileFilter.h"
#include <QFileInfo>
#include <QDebug>

namespace io {



// 支持的格式映射表
const FileFilter::FormatInfo FileFilter::formatMap[] = {
    {"OpenSceneGraph 模型", {"*.osg", "*.osga", "*.osgb", "*.osgt", "*.osgx", "*.ive"}},
    {"glTF 模型", {"*.gltf", "*.glb"}},
    {"LMB 模型", {"*.lmb"}},
    {"Wavefront OBJ", {"*.obj"}},
    {"Stanford PLY", {"*.ply"}},
    {"STL 模型", {"*.stl"}},
    {"FBX 模型", {"*.fbx"}},
    {"3DS 模型", {"*.3ds"}},
    {"Collada DAE", {"*.dae"}},
    {"AC3D 模型", {"*.ac"}},
    {"DXF 模型", {"*.dxf"}},
    {"LWO 模型", {"*.lwo"}},
    {"所有支持的文件", {"*.osg", "*.osga", "*.osgb", "*.osgt", "*.osgx", "*.ive", "*.gltf", "*.glb", "*.lmb", "*.obj", "*.ply", "*.stl", "*.fbx", "*.3ds", "*.dae", "*.ac", "*.dxf", "*.lwo"}}
};

QString FileFilter::buildFilterString() {
    QStringList filters;
    
    // 添加所有具体格式
    for (int i = 0; i < 11; ++i) {  // 更新为11个格式
        QString filter = formatMap[i].description + " (";
        filter += formatMap[i].extensions.join(" ");
        filter += ")";
        filters << filter;
    }
    
    // 添加"所有文件"选项
    filters << "所有文件 (*.*)";
    
    return filters.join(";;");
}

FileFilter::ModelFormat FileFilter::getFormatFromFile(const QString& filePath) {
    QFileInfo info(filePath);
    QString extension = info.suffix().toLower();
    
    // 检查每个格式的扩展名
    for (int i = 0; i < 11; ++i) {  // 更新为11个格式
        for (const QString& ext : formatMap[i].extensions) {
            if (ext.mid(2) == extension) {  // 去掉 "*."
                return static_cast<ModelFormat>(i);
            }
        }
    }
    
    return All_Formats;
}

QString FileFilter::getFormatDescription(ModelFormat format) {
    if (format >= 0 && format <= 6) {
        return formatMap[format].description;
    }
    return "未知格式";
}

QStringList FileFilter::getFormatExtensions(ModelFormat format) {
    if (format >= 0 && format <= 6) {
        return formatMap[format].extensions;
    }
    return QStringList();
}

bool FileFilter::isFormatSupported(const QString& filePath) {
    return getFormatFromFile(filePath) != All_Formats;
}

} // namespace io