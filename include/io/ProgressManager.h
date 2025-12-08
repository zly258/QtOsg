#ifndef PROGRESS_MANAGER_H
#define PROGRESS_MANAGER_H

#include <QObject>
#include <QProgressDialog>
#include <QTimer>
#include <QString>
#include <functional>
#include <memory>

// 进度管理命名空间
namespace progress {

// 进度回调函数类型
using ProgressCallback = std::function<void(int progress, const QString& message)>;

// 进度管理器 - 负责管理进度对话框和进度更新
class ProgressManager : public QObject {
    Q_OBJECT

public:
    explicit ProgressManager(QObject* parent = nullptr);
    ~ProgressManager();

    // 显示进度对话框
    void showProgressDialog(const QString& title = "正在处理...",
                           const QString& labelText = "请稍候...",
                           int minimum = 0, int maximum = 100);

    // 隐藏进度对话框
    void hideProgressDialog();

    // 更新进度
    void updateProgress(int value, const QString& message = QString());

    // 设置进度范围
    void setRange(int minimum, int maximum);

    // 检查是否显示进度对话框
    bool isProgressDialogVisible() const;

    // 创建异步进度回调
    ProgressCallback createAsyncCallback(int initialProgress = 0, 
                                        const QString& initialMessage = QString());

    // 设置自动隐藏延迟（毫秒）
    void setAutoHideDelay(int delayMs);

signals:
    void progressUpdated(int value, const QString& message);
    void progressFinished(bool success, const QString& message);

private slots:
    void onTimerTimeout();

private:
    class ProgressManagerPrivate;
    std::unique_ptr<ProgressManagerPrivate> d;
};

// 进度范围类 - 封装进度计算逻辑
class ProgressRange {
public:
    ProgressRange(int totalSteps, int startPercent = 0, int endPercent = 100);
    
    // 计算当前步骤的进度百分比
    int calculateProgress(int currentStep) const;
    
    // 计算当前步骤的进度文本
    QString calculateMessage(int currentStep, const QString& baseMessage) const;

private:
    int totalSteps_;
    int startPercent_;
    int endPercent_;
};

// 进度操作类 - 提供链式调用接口
class ProgressOperation {
public:
    ProgressOperation(ProgressManager* manager, const QString& operationName);
    ~ProgressOperation();

    // 设置进度范围
    ProgressOperation& setRange(int totalSteps);
    
    // 设置自定义进度回调
    ProgressOperation& setCallback(ProgressCallback callback);
    
    // 执行操作（同步版本）
    template<typename Func>
    bool execute(Func operation) {
        if (!manager_) return false;
        
        int totalSteps = totalSteps_;
        int completedSteps = 0;
        
        auto progressCallback = [this, &completedSteps, totalSteps](int progress, const QString& message) {
            int currentStep = (progress * totalSteps) / 100;
            if (currentStep > completedSteps) {
                completedSteps = currentStep;
                manager_->updateProgress(progress, QString("%1 (%2/%3)").arg(operationName_).arg(completedSteps).arg(totalSteps));
            }
        };
        
        try {
            // 执行操作
            bool result = operation(progressCallback);
            manager_->updateProgress(100, QString("%1 完成").arg(operationName_));
            return result;
        }
        catch (const std::exception& e) {
            manager_->updateProgress(0, QString("%1 失败: %2").arg(operationName_).arg(e.what()));
            return false;
        }
    }

private:
    ProgressManager* manager_;
    QString operationName_;
    int totalSteps_;
};

} // namespace progress

#endif // PROGRESS_MANAGER_H