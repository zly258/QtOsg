#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QAction>
#include <QDockWidget>
#include <QFileDialog>
#include <QObject>
#include <QMessageBox>
#include <QProgressDialog>
#include <QTimer>
#include <QTreeWidgetItem>
#include <vector>
#include <unordered_set>
#include <osg/Node>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Geode>
#include "OsgContainer.h"
#include "FileSettingsDialog.h"
#include "utils/SceneStructureParser.h"
#include "io/ModelLoader.h"
#include "io/FileFilter.h"
#include "io/ProgressManager.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    // 窗口显示后再统一调整一次 Dock 尺寸，避免初次布局时中央原生窗口过大
    void showEvent(QShowEvent *event) override;

private slots:
    void openFile();
    void updateLoadingProgress();
    void updateMemoryInfo();

    // 视图菜单
    void onViewFitToView();
    void onToggleSceneBBox(bool on);

    // 选择菜单
    void onToggleSinglePick(bool on);

    // 视图：清空场景
    void onClearScene();

    // OSG 视图选中/清空回调
    void onSelectionCleared();
    void onNodeSelected(osg::Node *node);

    // 导出相关槽函数
    void onExport();
    
    // 设置菜单
    void onFileLoadingSettings();

private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();

    // 内存相关函数
    QString getCurrentMemoryUsage();
    void loadModelFile(const QString &filePath);
    QString buildFileFilter();
    // 根据节点查找树项
    QTreeWidgetItem *findItemByNode(osg::Node *node);
    // 清空树选择
    void clearTreeSelection();
    // 遍历/可视工具
    void setNodeMaskRecursive(osg::Node *node, unsigned int mask);

    // 新增：增强功能相关方法
    void loadModelFileWithSettings(const QString &filePath);
    void loadModelFileWithCoordination(const QString &filePath, FileSettingsDialog::CoordinateSystem coordSystem);
    void loadModelWithIOFramework(const QString &filePath, FileSettingsDialog::CoordinateSystem coordSystem, double scaleFactor);
    void applyCoordinateSystem(osg::Node *node, FileSettingsDialog::CoordinateSystem coordSystem);
    void buildDetailedSceneTree(osg::Node *node, const QString &filePath);

private:
    // UI组件
    QWidget *centralWidget;               // 中央容器
    OsgContainer *osgContainer = nullptr; // OSG 视图

    // Dock：结构
    QDockWidget *structureDock; // 结构树 Dock

    // 结构树内容
    QTreeWidget *modelTreeWidget;

    // 菜单和状态栏
    QAction *openAction;
    QAction *exitAction;
    QAction *viewFitAction;
    QAction *viewBBoxAction;
    QAction *selectSingleAction;
    QAction *testExportLmbAction;
    QAction *clearSceneAction; // 清空场景（放在视图菜单下）

    // 导出菜单相关
    QAction *exportAction;
    
    // 设置菜单相关
    QAction *fileLoadingSettingsAction;

    // 进度相关
    QProgressDialog *progressDialog;
    QTimer *progressTimer;
    int currentProgress;

    // 内存显示相关
    QLabel *memoryLabel = nullptr;
    QTimer *memoryTimer = nullptr;

    // IO框架组件
    io::ModelLoader* modelLoader = nullptr;
    progress::ProgressManager* progressManager = nullptr;

    // 仅在首次显示时调整 Dock 尺寸
    bool docksSizedOnce = false;
};

#endif // MAINWINDOW_H