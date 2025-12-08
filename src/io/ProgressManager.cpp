#include "io/ProgressManager.h"
#include <QApplication>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

namespace progress {

// ProgressManager私有实现类
class ProgressManager::ProgressManagerPrivate {
public:
    QProgressDialog* dialog;
    QTimer* autoHideTimer;
    int autoHideDelay;
    bool isModal;

    ProgressManagerPrivate()
        : dialog(nullptr)
        , autoHideTimer(new QTimer())
        , autoHideDelay(1500)
        , isModal(true) {
        autoHideTimer->setSingleShot(true);
    }

    ~ProgressManagerPrivate() {
        if (dialog) {
            dialog->deleteLater();
        }
        autoHideTimer->deleteLater();
    }
};

ProgressManager::ProgressManager(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<ProgressManagerPrivate>()) {
    
    // 连接自动隐藏定时器
    QObject::connect(d->autoHideTimer, &QTimer::timeout, this, [this]() {
        hideProgressDialog();
    });
}

ProgressManager::~ProgressManager() = default;

void ProgressManager::showProgressDialog(const QString& title,
                                        const QString& labelText,
                                        int minimum, int maximum) {
    hideProgressDialog(); // 隐藏之前的对话框

    d->dialog = new QProgressDialog(labelText, QString(), minimum, maximum);
    d->dialog->setWindowTitle(title);
    d->dialog->setWindowModality(d->isModal ? Qt::WindowModal : Qt::NonModal);
    d->dialog->setMinimumDuration(0);
    d->dialog->setValue(0);
    d->dialog->setCancelButton(nullptr); // 禁用取消按钮
    d->dialog->setWindowFlags((d->dialog->windowFlags() & 
                              ~Qt::WindowContextHelpButtonHint & 
                              ~Qt::WindowCloseButtonHint) | 
                              Qt::FramelessWindowHint);
    
    d->dialog->show();
    QApplication::processEvents();
}

void ProgressManager::hideProgressDialog() {
    if (d->dialog) {
        d->dialog->close();
        d->dialog->deleteLater();
        d->dialog = nullptr;
    }
    d->autoHideTimer->stop();
}

void ProgressManager::updateProgress(int value, const QString& message) {
    if (d->dialog) {
        d->dialog->setValue(value);
        if (!message.isEmpty()) {
            d->dialog->setLabelText(message);
        }
        emit progressUpdated(value, message);
    }
    QApplication::processEvents();
}

void ProgressManager::setRange(int minimum, int maximum) {
    if (d->dialog) {
        d->dialog->setRange(minimum, maximum);
    }
}

bool ProgressManager::isProgressDialogVisible() const {
    return d->dialog && d->dialog->isVisible();
}

ProgressCallback ProgressManager::createAsyncCallback(int initialProgress, 
                                                    const QString& initialMessage) {
    return [this, initialProgress, initialMessage](int progress, const QString& message) {
        QString finalMessage = message.isEmpty() ? initialMessage : message;
        int finalProgress = qMax(initialProgress, progress);
        updateProgress(finalProgress, finalMessage);
    };
}

void ProgressManager::setAutoHideDelay(int delayMs) {
    d->autoHideDelay = delayMs;
}

void ProgressManager::onTimerTimeout() {
    hideProgressDialog();
}

// ProgressRange实现
ProgressRange::ProgressRange(int totalSteps, int startPercent, int endPercent)
    : totalSteps_(totalSteps)
    , startPercent_(startPercent)
    , endPercent_(endPercent) {
}

int ProgressRange::calculateProgress(int currentStep) const {
    if (totalSteps_ <= 0) return startPercent_;
    
    int stepProgress = (currentStep * 100) / totalSteps_;
    int totalRange = endPercent_ - startPercent_;
    
    return startPercent_ + (stepProgress * totalRange) / 100;
}

QString ProgressRange::calculateMessage(int currentStep, const QString& baseMessage) const {
    if (totalSteps_ <= 0) return baseMessage;
    
    return QString("%1 (%2/%3)").arg(baseMessage).arg(currentStep).arg(totalSteps_);
}

// ProgressOperation实现
ProgressOperation::ProgressOperation(ProgressManager* manager, const QString& operationName)
    : manager_(manager)
    , operationName_(operationName)
    , totalSteps_(100) {
}

ProgressOperation::~ProgressOperation() = default;

ProgressOperation& ProgressOperation::setRange(int totalSteps) {
    totalSteps_ = totalSteps;
    return *this;
}

ProgressOperation& ProgressOperation::setCallback(ProgressCallback callback) {
    // 可以在这里设置自定义回调
    return *this;
}

} // namespace progress