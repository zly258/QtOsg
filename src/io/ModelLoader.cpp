#include "io/ModelLoader.h"
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osg/MatrixTransform>
#include <osg/Matrix>
#include <osg/Group>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <memory>
#include <algorithm>
#include <functional>

namespace io {

// OSG模型读取器实现
class OsgModelReader : public IModelReader {
public:
    bool canLoad(const QString& filePath) const override {
        QFileInfo info(filePath);
        QString extension = info.suffix().toLower();
        
        // 支持的OSG格式
        QStringList supportedFormats = {
            "osg", "osga", "osgb", "osgt", "osgx", "ive",
            "obj", "ply", "stl", "3ds", "lwo", "x", "md2",
            "fbx", "dae", "ac", "dxf", "lws", "lwo2"
        };
        
        return supportedFormats.contains(extension);
    }

    LoadResult load(const QString& filePath, ProgressCallback progress = nullptr) override {
        LoadResult result;
        result.success = false;
        result.fileName = QFileInfo(filePath).fileName();

        try {
            if (progress) progress(10, "正在读取文件...");

            std::string stdPath = filePath.toLocal8Bit().constData();
            osg::ref_ptr<osg::Node> loadedNode = osgDB::readNodeFile(stdPath);

            if (progress) progress(30, "正在验证节点...");

            if (loadedNode.valid()) {
                result.success = true;
                result.node = std::shared_ptr<osg::Node>(loadedNode.get(), [](osg::Node*){});
                
                if (progress) progress(100, "加载完成");
                return result;
            } else {
                result.errorMessage = "无法读取文件格式或文件损坏";
            }
        }
        catch (const std::exception& e) {
            result.errorMessage = QString("加载异常: %1").arg(e.what());
        }

        if (progress) progress(0, "加载失败");
        return result;
    }

    QStringList getSupportedExtensions() const override {
        return {"osg", "osga", "osgb", "osgt", "osgx", "ive", "obj", "ply", "stl", "3ds", "lwo", "x", "md2", "fbx", "dae", "ac", "dxf", "lws", "lwo2"};
    }
};

// GLTF模型读取器实现
class GltfModelReader : public IModelReader {
public:
    bool canLoad(const QString& filePath) const override {
        QFileInfo info(filePath);
        QString extension = info.suffix().toLower();
        return extension == "gltf" || extension == "glb";
    }

    LoadResult load(const QString& filePath, ProgressCallback progress = nullptr) override {
        LoadResult result;
        result.success = false;
        result.fileName = QFileInfo(filePath).fileName();

        // 这里需要调用实际的GLTF插件
        // 暂时使用OSG的通用加载方法
        return loadViaOSG(filePath, progress);
    }

    QStringList getSupportedExtensions() const override {
        return {"gltf", "glb"};
    }

private:
    LoadResult loadViaOSG(const QString& filePath, ProgressCallback progress) {
        LoadResult result;
        result.success = false;
        result.fileName = QFileInfo(filePath).fileName();

        try {
            if (progress) progress(20, "正在解析GLTF格式...");

            std::string stdPath = filePath.toLocal8Bit().constData();
            osg::ref_ptr<osg::Node> loadedNode = osgDB::readNodeFile(stdPath);

            if (progress) progress(80, "正在构建场景图...");

            if (loadedNode.valid()) {
                result.success = true;
                result.node = std::shared_ptr<osg::Node>(loadedNode.get(), [](osg::Node*){});
                
                if (progress) progress(100, "GLTF加载完成");
                return result;
            } else {
                result.errorMessage = "GLTF解析失败或格式不支持";
            }
        }
        catch (const std::exception& e) {
            result.errorMessage = QString("GLTF加载异常: %1").arg(e.what());
        }

        if (progress) progress(0, "GLTF加载失败");
        return result;
    }
};

// 坐标系统转换器实现
class CoordinateSystemConverter : public ICoordinateSystemConverter {
public:
    std::shared_ptr<osg::MatrixTransform> convert(
        std::shared_ptr<osg::Node> node, 
        CoordinateSystem targetSystem) override {

        if (!node) return nullptr;

        osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform();
        transform->addChild(node.get());

        osg::Matrix matrix;
        switch (targetSystem) {
            case CoordinateSystem::Y_UP:
                matrix = osg::Matrix::identity();
                break;
            case CoordinateSystem::Z_UP:
                matrix = osg::Matrix::rotate(-osg::PI/2.0, 1.0, 0.0, 0.0);
                break;
            case CoordinateSystem::X_UP:
                matrix = osg::Matrix::rotate(osg::PI/2.0, 0.0, 1.0, 0.0);
                break;
            case CoordinateSystem::AutoDetect:
            default:
                matrix = osg::Matrix::identity();
                break;
        }

        transform->setMatrix(matrix);
        transform->setName("CoordinateSystem_Transformed");
        return std::shared_ptr<osg::MatrixTransform>(transform.get(), [](osg::MatrixTransform*){});
    }
};

// OSG模型写入器实现
class OsgModelWriter : public IModelWriter {
public:
    bool canWrite(const QString& format) const override {
        QStringList supportedFormats = {"osg", "osga", "osgb", "ive", "obj"};
        return supportedFormats.contains(format.toLower());
    }

    bool write(osg::Node* node, const QString& filePath, ProgressCallback progress = nullptr) override {
        if (!node) return false;

        try {
            if (progress) progress(20, "正在准备保存...");

            std::string stdPath = filePath.toLocal8Bit().constData();
            
            if (progress) progress(60, "正在写入文件...");

            bool success = osgDB::writeNodeFile(*node, stdPath);

            if (progress) progress(100, success ? "保存成功" : "保存失败");
            return success;
        }
        catch (const std::exception& e) {
            if (progress) progress(0, QString("保存异常: %1").arg(e.what()));
            return false;
        }
    }

    QStringList getSupportedFormats() const override {
        return {"osg", "osga", "osgb", "ive", "obj"};
    }
};

// ModelLoader私有实现
class ModelLoader::ModelLoaderPrivate {
public:
    std::vector<std::unique_ptr<IModelReader>> readers;
    std::vector<std::unique_ptr<IModelWriter>> writers;
    std::unique_ptr<ICoordinateSystemConverter> coordConverter;

    ModelLoaderPrivate() {
        // 注册默认的读取器和写入器
        readers.push_back(std::make_unique<OsgModelReader>());
        readers.push_back(std::make_unique<GltfModelReader>());
        writers.push_back(std::make_unique<OsgModelWriter>());
        coordConverter = std::make_unique<CoordinateSystemConverter>();
    }

    IModelReader* findReader(const QString& filePath) {
        for (auto& reader : readers) {
            if (reader->canLoad(filePath)) {
                return reader.get();
            }
        }
        return nullptr;
    }

    IModelWriter* findWriter(const QString& format) {
        for (auto& writer : writers) {
            if (writer->canWrite(format)) {
                return writer.get();
            }
        }
        return nullptr;
    }
};

ModelLoader::ModelLoader(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<ModelLoaderPrivate>()) {
}

ModelLoader::~ModelLoader() = default;

void ModelLoader::registerReader(std::unique_ptr<IModelReader> reader) {
    if (reader) {
        d->readers.push_back(std::move(reader));
    }
}

void ModelLoader::registerWriter(std::unique_ptr<IModelWriter> writer) {
    if (writer) {
        d->writers.push_back(std::move(writer));
    }
}

void ModelLoader::registerCoordinateConverter(std::unique_ptr<ICoordinateSystemConverter> converter) {
    if (converter) {
        d->coordConverter = std::move(converter);
    }
}

LoadResult ModelLoader::loadModel(const QString& filePath, 
                                  ProgressCallback progress,
                                  CoordinateSystem coordSystem) {
    emit loadingProgress(0, "开始加载模型...");

    // 查找合适的读取器
    auto reader = d->findReader(filePath);
    if (!reader) {
        LoadResult result;
        result.success = false;
        result.errorMessage = "不支持的文件格式";
        emit loadingFinished(false, result.errorMessage);
        return result;
    }

    // 执行加载
    auto result = reader->load(filePath, progress);

    if (result.success && result.node && coordSystem != CoordinateSystem::AutoDetect) {
        // 应用坐标系统转换
        if (progress) progress(85, "正在转换坐标系统...");
        
        auto transformed = d->coordConverter->convert(result.node, coordSystem);
        if (transformed) {
            result.node = transformed;
        }
    }

    emit loadingFinished(result.success, result.errorMessage);
    return result;
}

bool ModelLoader::saveModel(osg::Node* node, const QString& filePath, 
                           const QString& format, ProgressCallback progress) {
    auto writer = d->findWriter(format);
    if (!writer) {
        if (progress) progress(0, "不支持的输出格式");
        return false;
    }

    return writer->write(node, filePath, progress);
}

QStringList ModelLoader::getSupportedReadFormats() const {
    QStringList formats;
    for (const auto& reader : d->readers) {
        formats << reader->getSupportedExtensions();
    }
    formats.removeDuplicates();
    return formats;
}

QStringList ModelLoader::getSupportedWriteFormats() const {
    QStringList formats;
    for (const auto& writer : d->writers) {
        formats << writer->getSupportedFormats();
    }
    formats.removeDuplicates();
    return formats;
}

} // namespace io