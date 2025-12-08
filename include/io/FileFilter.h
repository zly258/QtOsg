#ifndef FILE_FILTER_H
#define FILE_FILTER_H

#include <QString>
#include <QStringList>

namespace io {

// 文件过滤器类，负责构建文件对话框的过滤器字符串
class FileFilter {
public:
    // 支持的模型格式
    enum ModelFormat {
        OSG_Format,     // OpenSceneGraph (.osg, .osga, .osgb, .osgt, .osgx, .ive)
        GLTF_Format,    // glTF (.gltf, .glb)
        LMB_Format,     // LMB格式 (.lmb)
        OBJ_Format,     // Wavefront OBJ (.obj)
        PLY_Format,     // Stanford PLY (.ply)
        STL_Format,     // STL (.stl)
        FBX_Format,     // FBX (.fbx)
        _3DS_Format,    // 3DS (.3ds)
        DAE_Format,     // Collada DAE (.dae)
        AC3D_Format,    // AC3D (.ac)
        DXF_Format,     // DXF (.dxf)
        LWO_Format,     // LightWave (.lwo)
        All_Formats     // 所有支持的格式
    };

    // 构建文件过滤器字符串
    static QString buildFilterString();

    // 根据文件路径判断格式
    static ModelFormat getFormatFromFile(const QString& filePath);

    // 获取格式描述
    static QString getFormatDescription(ModelFormat format);

    // 获取格式扩展名列表
    static QStringList getFormatExtensions(ModelFormat format);

    // 检查文件格式是否支持
    static bool isFormatSupported(const QString& filePath);

private:
    struct FormatInfo {
        QString description;
        QStringList extensions;
    };

    static const FormatInfo formatMap[];
};

} // namespace io

#endif // FILE_FILTER_H