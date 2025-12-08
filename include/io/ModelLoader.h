#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <memory>
#include <functional>
#include <QString>
#include <QObject>

namespace osg {
    class Node;
    class Group;
    class MatrixTransform;
}

namespace io {

// 进度回调函数类型
using ProgressCallback = std::function<void(int progress, const QString& message)>;

// 模型加载结果
struct LoadResult {
    bool success;
    QString errorMessage;
    std::shared_ptr<osg::Node> node;
    QString fileName;
};

// 坐标系统类型
enum class CoordinateSystem {
    AutoDetect,
    Y_UP,
    Z_UP,
    X_UP
};

// 模型读取器接口
class IModelReader {
public:
    virtual ~IModelReader() = default;
    virtual bool canLoad(const QString& filePath) const = 0;
    virtual LoadResult load(const QString& filePath, ProgressCallback progress = nullptr) = 0;
    virtual QStringList getSupportedExtensions() const = 0;
};

// 模型写入器接口
class IModelWriter {
public:
    virtual ~IModelWriter() = default;
    virtual bool canWrite(const QString& format) const = 0;
    virtual bool write(osg::Node* node, const QString& filePath, ProgressCallback progress = nullptr) = 0;
    virtual QStringList getSupportedFormats() const = 0;
};

// 坐标系统转换器接口
class ICoordinateSystemConverter {
public:
    virtual ~ICoordinateSystemConverter() = default;
    virtual std::shared_ptr<osg::MatrixTransform> convert(
        std::shared_ptr<osg::Node> node, 
        CoordinateSystem targetSystem) = 0;
};

// 主要的模型加载器类
class ModelLoader : public QObject {
    Q_OBJECT

public:
    explicit ModelLoader(QObject* parent = nullptr);
    ~ModelLoader();

    // 注册读取器
    void registerReader(std::unique_ptr<IModelReader> reader);
    void registerWriter(std::unique_ptr<IModelWriter> writer);
    void registerCoordinateConverter(std::unique_ptr<ICoordinateSystemConverter> converter);

    // 加载模型
    LoadResult loadModel(const QString& filePath, 
                        ProgressCallback progress = nullptr,
                        CoordinateSystem coordSystem = CoordinateSystem::AutoDetect);

    // 保存模型
    bool saveModel(osg::Node* node, const QString& filePath, 
                  const QString& format = "osg",
                  ProgressCallback progress = nullptr);

    // 获取支持的格式
    QStringList getSupportedReadFormats() const;
    QStringList getSupportedWriteFormats() const;

signals:
    void loadingProgress(int progress, const QString& message);
    void loadingFinished(bool success, const QString& message);

private:
    class ModelLoaderPrivate;
    std::unique_ptr<ModelLoaderPrivate> d;
};

} // namespace io

#endif // MODEL_LOADER_H