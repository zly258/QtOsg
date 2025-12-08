#include "view/MainWindow.h"
#include "utils/StringUtil.h"
#include "io/ModelLoader.h"
#include "io/FileFilter.h"
#include "io/ProgressManager.h"
#include "view/FileSettingsDialog.h"

#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <QApplication>
#include <QTreeWidgetItem>
#include <QDebug>
#include <QTimer>
#include <QSettings>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QInputDialog>
#include <QFile>
#include <QByteArray>
#include <QDataStream>
#include <QThread>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QTextCodec>
#include <cmath>
#include <osgDB/WriteFile>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

#include <osgDB/ReadFile>
#include <osgUtil/Optimizer>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Uniform>
#include <osg/Camera>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), progressDialog(nullptr), progressTimer(nullptr), currentProgress(0),
      memoryLabel(nullptr), memoryTimer(nullptr), modelLoader(nullptr),
      progressManager(nullptr)
{
    setupUI();
    setupMenuBar();
    setupStatusBar();
    // 去除主窗口标题栏"？"帮助按钮
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // 初始化进度相关组件
    progressTimer = new QTimer(this);
    connect(progressTimer, &QTimer::timeout, this, &MainWindow::updateLoadingProgress);

    // 初始化IO框架组件
    progressManager = new progress::ProgressManager(this);
    modelLoader = new io::ModelLoader(this);

    // 设置OSG插件搜索路径
    osgDB::Registry *registry = osgDB::Registry::instance();

    // 添加当前应用程序目录下的osgPlugins目录
    QString appDir = QApplication::applicationDirPath();
    QString pluginDir = appDir + "/osgPlugins-3.6.5";
    registry->getLibraryFilePathList().push_back(pluginDir.toStdString());

    // 添加构建目录中的插件路径（用于开发调试）
    QString buildPluginDir = appDir + "/../plugins/osgdb_lmb/Debug";
    registry->getLibraryFilePathList().push_back(buildPluginDir.toStdString());

    buildPluginDir = appDir + "/../plugins/osgdb_gltf/Debug";
    registry->getLibraryFilePathList().push_back(buildPluginDir.toStdString());

    // 尝试显式加载插件
    registry->loadLibrary("osgdb_lmb");
    registry->loadLibrary("osgdb_gltf");
}

// 视图：充满视图
void MainWindow::onViewFitToView()
{
    if (osgContainer)
        osgContainer->fitToView();
}

// 视图：显示/隐藏场景包围盒
void MainWindow::onToggleSceneBBox(bool on)
{
    if (osgContainer)
        osgContainer->setShowSceneBBox(on);
}

// 视图：清空场景（包含左侧结构树）
void MainWindow::onClearScene()
{
    if (osgContainer)
    {
        osg::Group *root = osgContainer->getRoot();
        if (root)
            root->removeChildren(0, root->getNumChildren());
    }
    if (modelTreeWidget)
        modelTreeWidget->clear();
    statusBar()->showMessage("已清空场景", 1500);
}

// 状态栏初始化（若已初始化则跳过重复添加）
void MainWindow::setupStatusBar()
{
    // 将状态栏设为原生窗口，避免被中央原生子窗口遮挡
    this->statusBar()->setAttribute(Qt::WA_NativeWindow);
    // 状态栏仅显示一条就绪信息
    this->statusBar()->showMessage(QString("就绪"));
    if (!memoryLabel)
    {
        memoryLabel = new QLabel("内存: N/A", this);
        memoryLabel->setMinimumWidth(160);
        this->statusBar()->addPermanentWidget(memoryLabel);
    }
    // 定时低频刷新内存占用，避免影响UI
    if (!memoryTimer)
    {
        memoryTimer = new QTimer(this);
        memoryTimer->setInterval(1000);
        connect(memoryTimer, &QTimer::timeout, this, &MainWindow::updateMemoryInfo);
        memoryTimer->start();
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    // 中央视图：OSG 视图
    osgContainer = new OsgContainer();
    osgContainer->setMinimumSize(400, 300);
    setCentralWidget(osgContainer);
    // 连接 OSG 视图的单选信号
    if (osgContainer)
    {
        connect(osgContainer, &OsgContainer::nodePicked, this, &MainWindow::onNodeSelected);
        connect(osgContainer, &OsgContainer::selectionCleared, this, &MainWindow::onSelectionCleared);
    }

    // 左侧结构树 Dock
    structureDock = new QDockWidget(QString("模型结构"), this);
    structureDock->setObjectName("StructureDock");
    structureDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    // 仅允许移动，不可浮动，不显示关闭按钮
    structureDock->setFeatures(QDockWidget::DockWidgetMovable);
    // 重要：将 Dock 设为原生窗口，避免被 createWindowContainer 产生的原生子窗口遮挡
    structureDock->setAttribute(Qt::WA_NativeWindow);
    modelTreeWidget = new QTreeWidget(structureDock);
    modelTreeWidget->setHeaderLabel(QString("模型结构"));
    modelTreeWidget->setHeaderHidden(true); // 不显示表头
    modelTreeWidget->setMinimumWidth(220);
    // 连接树选择事件到3D视图高亮
    connect(modelTreeWidget, &QTreeWidget::itemSelectionChanged, [this]()
            {
        // 先清空之前的高亮
        if (osgContainer) {
            osgContainer->clearHighlight();
        }
        
        QList<QTreeWidgetItem*> selectedItems = modelTreeWidget->selectedItems();
        if (!selectedItems.isEmpty()) {
            QTreeWidgetItem* item = selectedItems.first();
            QVariant nodeData = item->data(0, Qt::UserRole);
            osg::Node* node = reinterpret_cast<osg::Node*>(nodeData.value<void*>());
            
            // 在3D视图中高亮选中的节点
            if (osgContainer && node) {
                osgContainer->highlightNode(node);
            }
        } });
    // 结构树不使用展开懒加载
    structureDock->setWidget(modelTreeWidget);
    addDockWidget(Qt::LeftDockWidgetArea, structureDock);

    setWindowTitle("轻量化模型浏览器");
    resize(1200, 800);
}

void MainWindow::setupMenuBar()
{
    // 文件 菜单（打开/退出）
    // 将菜单栏设为原生窗口，避免被中央原生子窗口遮挡
    this->menuBar()->setAttribute(Qt::WA_NativeWindow);
    QMenu *fileMenu = this->menuBar()->addMenu(QString("文件(&F)"));

    openAction = new QAction(QString("打开(&O)"), this);
    openAction->setShortcut(QKeySequence::Open);
    openAction->setStatusTip(QString("打开模型文件"));
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    fileMenu->addAction(openAction);

    // 添加导出菜单项到文件菜单
    exportAction = new QAction(QString("导出(&E)"), this);
    exportAction->setShortcut(QKeySequence("Ctrl+E"));
    exportAction->setStatusTip(QString("导出模型到指定格式"));
    exportAction->setEnabled(false); // 初始禁用，有模型时启用
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExport);
    fileMenu->addAction(exportAction);

    fileMenu->addSeparator();

    exitAction = new QAction(QString("退出(&X)"), this);
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setStatusTip(QString("退出应用程序"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);

    // 视图 菜单
    QMenu *viewMenu = this->menuBar()->addMenu(QString("视图(&V)"));
    viewFitAction = viewMenu->addAction(QString("充满视图"));
    connect(viewFitAction, &QAction::triggered, this, &MainWindow::onViewFitToView);
    viewBBoxAction = viewMenu->addAction(QString("显示包围盒"));
    viewBBoxAction->setCheckable(true);
    viewBBoxAction->setChecked(false);
    connect(viewBBoxAction, &QAction::toggled, this, &MainWindow::onToggleSceneBBox);

    // 清空场景
    clearSceneAction = viewMenu->addAction(QString("清空场景"));
    connect(clearSceneAction, &QAction::triggered, this, &MainWindow::onClearScene);

    // 选择 菜单
    QMenu *selectMenu = this->menuBar()->addMenu(QString("选择(&S)"));
    selectSingleAction = selectMenu->addAction(QString("启用单选"));
    selectSingleAction->setCheckable(true);
    selectSingleAction->setChecked(false);
    connect(selectSingleAction, &QAction::toggled, this, &MainWindow::onToggleSinglePick);
    
    // 设置 菜单
    QMenu *settingsMenu = this->menuBar()->addMenu(QString("设置(&T)"));
    fileLoadingSettingsAction = settingsMenu->addAction(QString("文件加载设置"));
    fileLoadingSettingsAction->setStatusTip(QString("配置文件加载选项"));
    connect(fileLoadingSettingsAction, &QAction::triggered, this, &MainWindow::onFileLoadingSettings);
}

void MainWindow::openFile()
{
    // 使用 QSettings 记录和读取上次打开的目录
    QSettings settings("LMBModelViewer", "LMBModelViewer");
    QString lastDir = settings.value("lastOpenDir", QDir::homePath()).toString();

    // 使用IO框架的FileFilter获取文件过滤器
    QString filter = io::FileFilter::buildFilterString();

    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "打开模型文件(支持多选)",
        lastDir,
        filter);

    if (!files.isEmpty())
    {
        // 记住本次打开的目录
        settings.setValue("lastOpenDir", QFileInfo(files.first()).absolutePath());
        for (const QString &f : files)
        {
            loadModelFileWithSettings(f);
        }
    }
}

// 使用IO框架进行模型加载的方法
void MainWindow::loadModelWithIOFramework(const QString &filePath, 
                                          FileSettingsDialog::CoordinateSystem coordSystem,
                                          double scaleFactor)
{
    if (!modelLoader || !progressManager) {
        // 如果IO组件不可用，回退到原有方法
        loadModelFileWithCoordination(filePath, coordSystem);
        return;
    }

    try {
        // 使用ProgressManager显示进度
    auto progressCallback = [this](int progress, const QString& message) {
        if (progressManager) {
            progressManager->updateProgress(progress, message);
        }
    };
    
    // 调用ModelLoader进行模型加载
    io::LoadResult result = modelLoader->loadModel(filePath, progressCallback);
        
        if (result.success && result.node) {
            // 应用坐标系统
            progressCallback(30, "正在应用坐标系统...");
            applyCoordinateSystem(result.node.get(), coordSystem);
            
            // 应用缩放因子
            if (scaleFactor != 1.0) {
                progressCallback(45, "正在应用缩放...");
                osg::ref_ptr<osg::MatrixTransform> scaleTransform = new osg::MatrixTransform();
                scaleTransform->addChild(result.node.get());
                osg::Matrix scaleMatrix;
                scaleMatrix.makeScale(scaleFactor, scaleFactor, scaleFactor);
                scaleTransform->setMatrix(scaleMatrix);
                result.node = std::shared_ptr<osg::Node>(scaleTransform.get(), [scaleTransform](osg::Node*) mutable { scaleTransform.release(); });
            }
            
            // 默认使用追加模式添加到场景
            progressCallback(60, "正在添加模型到场景...");
            
            // 将加载的节点包装到Group中（如果不是Group的话）
            osg::ref_ptr<osg::Group> model;
            if (osg::Group *group = result.node.get()->asGroup()) {
                model = group;
            } else {
                model = new osg::Group();
                model->addChild(result.node.get());
            }
            
            // 添加模型到场景
            if (osgContainer && osgContainer->getRoot()) {
                osgContainer->getRoot()->addChild(model);
            }
            
            // 设置节点掩码以支持拾取
            progressCallback(85, "正在设置节点属性...");
            if (model) {
                this->setNodeMaskRecursive(model.get(), 0xFFu);
            }
            
            // 构建模型结构树
            progressCallback(90, "正在构建模型树...");
            this->buildDetailedSceneTree(model.get(), filePath);
            
            // 完成加载
            progressCallback(100, "加载完成");
            if (progressManager) {
                progressManager->hideProgressDialog();
            }
            
            // 启用导出菜单项
            if (exportAction) {
                exportAction->setEnabled(true);
            }
            
            QString successMsg = QString("模型加载成功: %1").arg(QFileInfo(filePath).fileName());
            if (statusBar()) {
                statusBar()->showMessage(successMsg, 3000);
            }
        } else {
            // 加载失败
            if (progressManager) {
                progressManager->hideProgressDialog();
            }
            
            QString errorMsg = QString("无法加载模型文件:\n%1\n\n错误: %2")
                .arg(QFileInfo(filePath).fileName())
                .arg(result.errorMessage);
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("加载失败");
            msgBox.setText(errorMsg);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.exec();
            
            if (statusBar()) {
                statusBar()->showMessage("模型加载失败", 3000);
            }
        }
    } catch (const std::exception &e) {
        // 异常处理
        if (progressManager) {
            progressManager->hideProgressDialog();
        }
        
        QString exceptionMsg = QString::fromUtf8(e.what());
        QString errorMsg = QString("加载模型时发生异常:\n%1\n\n文件: %2")
                               .arg(exceptionMsg)
                               .arg(QFileInfo(filePath).fileName());
        
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("加载异常");
        msgBox.setText(errorMsg);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        
        if (statusBar()) {
            statusBar()->showMessage("模型加载失败", 3000);
        }
    }
}

// 带设置的模型加载方法
void MainWindow::loadModelFileWithSettings(const QString &filePath)
{
    // 使用静态方法获取设置，不显示对话框
    FileSettingsDialog::CoordinateSystem coordSystem;
    FileSettingsDialog::UnitType unitType;
    double scaleFactor;
    
    if (!FileSettingsDialog::getStaticSettings(coordSystem, unitType, scaleFactor)) {
        return;
    }
    
    // 使用IO框架的ModelLoader进行模型加载（默认追加模式）
    if (modelLoader && progressManager) {
        loadModelWithIOFramework(filePath, static_cast<FileSettingsDialog::CoordinateSystem>(coordSystem), scaleFactor);
    } else {
        // 回退到原有方法
        loadModelFileWithCoordination(filePath, static_cast<FileSettingsDialog::CoordinateSystem>(coordSystem));
    }
}

// 改进的模型加载方法，支持坐标系统设置和高级结构树
void MainWindow::loadModelFileWithCoordination(const QString &filePath, FileSettingsDialog::CoordinateSystem coordSystem)
{
    // 确保文件路径使用UTF-8编码
    QString normalizedPath = QDir::toNativeSeparators(filePath);

    // 显示进度对话框
    this->progressDialog = new QProgressDialog("", "", 0, 100, this);
    this->progressDialog->setWindowModality(Qt::WindowModal);
    this->progressDialog->setMinimumDuration(0);
    this->progressDialog->setValue(0);
    this->progressDialog->setWindowFlags((this->progressDialog->windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowCloseButtonHint) | Qt::FramelessWindowHint);
    this->progressDialog->setCancelButton(nullptr);
    this->progressDialog->setLabelText("正在加载模型...");
    this->progressDialog->show();

    QString statusMsg = QString("正在加载模型文件...");
    this->statusBar()->showMessage(statusMsg);

    // 处理界面事件，确保进度对话框显示
    QApplication::processEvents();

    try
    {
        // 使用OSG标准API加载模型
        std::string stdFilePath = StringUtil::QStringToLocalPath(normalizedPath);
        osg::ref_ptr<osg::Node> loadedNode = osgDB::readNodeFile(stdFilePath);

        if (loadedNode && loadedNode.valid())
        {
            if (this->progressDialog)
            {
                this->progressDialog->setLabelText("正在处理坐标系统...");
                this->progressDialog->setValue(30);
                QApplication::processEvents();
            }

            // 应用坐标系统设置
            this->applyCoordinateSystem(loadedNode.get(), coordSystem);

            if (this->progressDialog)
            {
                this->progressDialog->setLabelText("正在添加模型到场景...");
                this->progressDialog->setValue(60);
                QApplication::processEvents();
            }

            // 将加载的节点包装到Group中（如果不是Group的话）
            osg::ref_ptr<osg::Group> model;
            if (osg::Group *group = loadedNode->asGroup())
            {
                model = group;
            }
            else
            {
                model = new osg::Group();
                model->addChild(loadedNode);
            }

            // 添加模型到场景（追加模式，不清空现有模型）
            this->osgContainer->getRoot()->addChild(model);

            if (this->progressDialog)
            {
                this->progressDialog->setLabelText("正在构建模型树...");
                this->progressDialog->setValue(90);
                QApplication::processEvents();
            }

            // 使用新的SceneStructureParser构建详细结构树
            this->buildDetailedSceneTree(model.get(), normalizedPath);

            // 设置节点掩码以支持拾取
            this->setNodeMaskRecursive(model.get(), 0xFFu);

            if (this->progressDialog)
            {
                this->progressDialog->setValue(100);
                this->progressDialog->close();
                delete this->progressDialog;
                this->progressDialog = nullptr;
            }

            // 启用导出菜单项
            if (this->exportAction) {
                this->exportAction->setEnabled(true);
            }

            QString successMsg = QString("模型加载成功");
            this->statusBar()->showMessage(successMsg, 3000);
        }
        else
        {
            if (this->progressDialog)
            {
                this->progressDialog->close();
                delete this->progressDialog;
                this->progressDialog = nullptr;
            }

            QString extension = QFileInfo(normalizedPath).suffix().toLower();
            QString errorMsg = QString("无法加载模型文件:\n%1\n\n可能的原因:\n• 不支持的文件格式 (.%2)\n• 文件损坏或格式不正确\n• 文件路径包含不支持的字符\n• 内存不足\n• 缺少相应的OSG插件")
                                   .arg(QFileInfo(normalizedPath).fileName())
                                   .arg(extension);

            QMessageBox msgBox(this);
            msgBox.setWindowTitle("加载失败");
            msgBox.setText(errorMsg);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.exec();

            this->statusBar()->showMessage("模型加载失败", 3000);
        }
    }
    catch (const std::exception &e)
    {
        if (this->progressDialog)
        {
            this->progressDialog->close();
            delete this->progressDialog;
            this->progressDialog = nullptr;
        }

        QString exceptionMsg = QString::fromUtf8(e.what());
        QString errorMsg = QString("加载模型时发生异常:\n%1\n\n文件: %2")
                               .arg(exceptionMsg)
                               .arg(QFileInfo(normalizedPath).fileName());

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("加载异常");
        msgBox.setText(errorMsg);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();

        this->statusBar()->showMessage("模型加载失败", 3000);
    }
}

// 应用坐标系统设置
void MainWindow::applyCoordinateSystem(osg::Node *node, FileSettingsDialog::CoordinateSystem coordSystem)
{
    if (!node) return;
    
    // 根据选择的坐标系统对模型进行变换
    osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform();
    transform->addChild(node);
    
    osg::Matrix matrix;
    switch (static_cast<int>(coordSystem))
    {
        case static_cast<int>(FileSettingsDialog::Y_UP):
            // 默认Y向上，不需要变换
            matrix = osg::Matrix::identity();
            break;
        case static_cast<int>(FileSettingsDialog::Z_UP):
            // Z向上：绕X轴旋转-90度
            matrix = osg::Matrix::rotate(-osg::PI/2.0, 1.0, 0.0, 0.0);
            break;
        default:
            // 自动检测：保持原样
            matrix = osg::Matrix::identity();
            break;
    }
    
    transform->setMatrix(matrix);
    
    // 替换原节点
    osg::Group* parent = node->getParent(0);
    if (parent)
    {
        int childIndex = parent->getChildIndex(node);
        parent->replaceChild(node, transform);
        transform->setName("CoordinateSystem_Transformed");
    }
}

// 构建详细的场景结构树
void MainWindow::buildDetailedSceneTree(osg::Node *node, const QString &filePath)
{
    // 创建场景结构解析器
    SceneStructureParser parser;
    
    // 使用解析器构建结构树
    parser.parseAndBuildTree(node, modelTreeWidget);
}

// 改进的节点选择处理，支持更好的视觉效果
void MainWindow::onNodeSelected(osg::Node *node)
{
    if (!node)
        return;

    // 在状态栏显示选中的节点信息
    QString nodeName = QString::fromStdString(node->getName());
    if (nodeName.isEmpty())
    {
        nodeName = QString::fromStdString(node->className());
    }

    this->statusBar()->showMessage(QString("选中: %1").arg(nodeName));

    // 在树形视图中高亮对应的项
    if (modelTreeWidget)
    {
        // 清空现有选择
        clearTreeSelection();
        // 查找并选中对应项
        QTreeWidgetItem *it = findItemByNode(node);
        if (it)
        {
            it->setSelected(true);
            modelTreeWidget->scrollToItem(it);
        }
    }

    qDebug() << "Node selected from 3D view:" << nodeName;
}

// 改进的高亮清除方法
void MainWindow::onSelectionCleared()
{
    // 清除3D视图中的高亮
    if (osgContainer)
    {
        osgContainer->clearHighlight();
    }
    
    // 清除树形视图中的选择
    clearTreeSelection();
    
    this->statusBar()->showMessage("已清除选择", 1500);
}

// 清空树形视图选择
void MainWindow::clearTreeSelection()
{
    if (modelTreeWidget)
    {
        modelTreeWidget->blockSignals(true);
        modelTreeWidget->clearSelection();
        modelTreeWidget->blockSignals(false);
    }
}

void MainWindow::loadModelFile(const QString &filePath)
{
    // 确保文件路径使用UTF-8编码
    QString normalizedPath = QDir::toNativeSeparators(filePath);

    // 显示进度对话框
    progressDialog = new QProgressDialog("", "", 0, 100, this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setMinimumDuration(0);
    progressDialog->setValue(0);
    progressDialog->setWindowFlags((progressDialog->windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowCloseButtonHint) | Qt::FramelessWindowHint);
    progressDialog->setCancelButton(nullptr);
    progressDialog->setLabelText("正在加载模型...");
    progressDialog->show();

    QString statusMsg = QString("正在加载模型文件...");
    this->statusBar()->showMessage(statusMsg);

    // 处理界面事件，确保进度对话框显示
    QApplication::processEvents();

    try
    {
        // 使用OSG标准API加载模型
        std::string stdFilePath = StringUtil::QStringToLocalPath(normalizedPath);
        osg::ref_ptr<osg::Node> loadedNode = osgDB::readNodeFile(stdFilePath);

        if (loadedNode && loadedNode.valid())
        {
            if (progressDialog)
            {
                progressDialog->setLabelText("正在添加模型到场景...");
                progressDialog->setValue(80);
                QApplication::processEvents();
            }

            // 将加载的节点包装到Group中（如果不是Group的话）
            osg::ref_ptr<osg::Group> model;
            if (osg::Group *group = loadedNode->asGroup())
            {
                model = group;
            }
            else
            {
                model = new osg::Group();
                model->addChild(loadedNode);
            }

            // 添加模型到场景（追加模式，不清空现有模型）
            osgContainer->getRoot()->addChild(model);

            if (progressDialog)
            {
                progressDialog->setLabelText("正在构建模型树...");
                progressDialog->setValue(90);
                QApplication::processEvents();
            }

            // 只创建一个简单的文件节点
            QTreeWidgetItem *rootItem = new QTreeWidgetItem(modelTreeWidget);
            QString fileName = QFileInfo(normalizedPath).baseName();
            rootItem->setText(0, fileName);
            rootItem->setExpanded(true);
            rootItem->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void *>(model.get())));
            // 设置节点掩码以支持拾取
            setNodeMaskRecursive(model.get(), 0xFFu);

            if (progressDialog)
            {
                progressDialog->setValue(100);
                progressDialog->close();
                delete progressDialog;
                progressDialog = nullptr;
            }

            // 启用导出菜单项
            exportAction->setEnabled(true);

            QString successMsg = QString("模型加载成功");
            this->statusBar()->showMessage(successMsg, 3000);
        }
        else
        {
            if (progressDialog)
            {
                progressDialog->close();
                delete progressDialog;
                progressDialog = nullptr;
            }

            QString extension = QFileInfo(normalizedPath).suffix().toLower();
            QString errorMsg = QString("无法加载模型文件:\n%1\n\n可能的原因:\n• 不支持的文件格式 (.%2)\n• 文件损坏或格式不正确\n• 文件路径包含不支持的字符\n• 内存不足\n• 缺少相应的OSG插件")
                                   .arg(QFileInfo(normalizedPath).fileName())
                                   .arg(extension);

            QMessageBox msgBox(this);
            msgBox.setWindowTitle("加载失败");
            msgBox.setText(errorMsg);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.exec();

            this->statusBar()->showMessage("模型加载失败", 3000);
        }
    }
    catch (const std::exception &e)
    {
        if (progressDialog)
        {
            progressDialog->close();
            delete progressDialog;
            progressDialog = nullptr;
        }

        QString exceptionMsg = QString::fromUtf8(e.what());
        QString errorMsg = QString("加载模型时发生异常:\n%1\n\n文件: %2")
                               .arg(exceptionMsg)
                               .arg(QFileInfo(normalizedPath).fileName());

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("加载异常");
        msgBox.setText(errorMsg);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();

        this->statusBar()->showMessage("模型加载失败", 3000);
    }
}

// 辅助：根据 osg::Node* 查找树项
QTreeWidgetItem *MainWindow::findItemByNode(osg::Node *node)
{
    if (!node)
        return nullptr;
    // 广度优先遍历树
    QList<QTreeWidgetItem *> queue;
    for (int i = 0; i < modelTreeWidget->topLevelItemCount(); ++i)
    {
        queue.append(modelTreeWidget->topLevelItem(i));
    }
    while (!queue.isEmpty())
    {
        QTreeWidgetItem *it = queue.takeFirst();
        QVariant v = it->data(0, Qt::UserRole);
        osg::Node *stored = reinterpret_cast<osg::Node *>(v.value<void *>());
        if (stored == node)
            return it;
        for (int i = 0; i < it->childCount(); ++i)
            queue.append(it->child(i));
    }
    return nullptr;
}

void MainWindow::onToggleSinglePick(bool on)
{
    if (osgContainer)
    {
        osgContainer->setSinglePickEnabled(on);
        if (!on)
        {
            // 关闭单选时清空一次选择与高亮
            osgContainer->clearHighlight();
            onSelectionCleared();
        }
    }
}

void MainWindow::setNodeMaskRecursive(osg::Node *node, unsigned int mask)
{
    if (!node)
        return;
    node->setNodeMask(mask);
    osg::Group *g = node->asGroup();
    if (g)
    {
        for (unsigned int i = 0; i < g->getNumChildren(); ++i)
        {
            setNodeMaskRecursive(g->getChild(i), mask);
        }
    }
}

void MainWindow::updateLoadingProgress()
{
    if (progressDialog)
    {
        currentProgress += 10;
        progressDialog->setValue(currentProgress);

        if (currentProgress >= 100)
        {
            progressTimer->stop();
            progressDialog->close();
            currentProgress = 0;
        }
    }
}

// 获取当前进程内存使用量
QString MainWindow::getCurrentMemoryUsage()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc)))
    {
        // 获取工作集大小（物理内存）
        double workingSetMB = pmc.WorkingSetSize / 1024.0 / 1024.0;
        return QString("内存: %1 MB").arg(QString::number(workingSetMB, 'f', 1));
    }
#endif
    return QString("内存: N/A");
}

// 更新内存信息显示
void MainWindow::updateMemoryInfo()
{
    if (memoryLabel)
    {
        memoryLabel->setText(getCurrentMemoryUsage());
    }
}

// 首次显示时调整 Dock 尺寸，优化与中央原生窗口的布局关系
void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (!docksSizedOnce)
    {

        docksSizedOnce = true;
    }
}

// 设置菜单 - 文件加载设置
void MainWindow::onFileLoadingSettings()
{
    FileSettingsDialog dialog(this);
    dialog.exec();
}



QString MainWindow::buildFileFilter()
{
    // 支持的文件格式（通过OSG插件系统）
    QStringList filters;

    // 1. 所有支持的格式
    QStringList allPatterns = {
        // 项目自定义格式
        "*.lmb",                             // LMB格式（通过插件）
        // 现代3D格式
        "*.gltf", "*.glb",                   // GLTF格式（通过插件）
        // 传统3D格式
        "*.obj",                             // Wavefront OBJ
        "*.3ds",                             // 3D Studio Max
        "*.dae",                             // COLLADA
        "*.fbx",                             // Filmbox (需要插件)
        "*.ply",                             // Stanford PLY
        "*.stl",                             // STL
        "*.x",                               // DirectX X
        // 游戏引擎格式
        "*.md2",                             // Quake 2
        "*.md3",                             // Quake 3
        "*.bsp",                             // Quake BSP
        // 专业格式
        "*.flt",                             // OpenFlight
        "*.shp",                             // ESRI Shape
        "*.txp",                             // TerraXport
        // OSG原生格式
        "*.osg", "*.osgt", "*.osgb", "*.ive" // OSG原生格式
    };
    filters.append(QString("All Supported (%1)").arg(allPatterns.join(" ")));

    // 2. 按格式类型分组的过滤器
    filters.append("LMB Files (*.lmb)");
    filters.append("GLTF/GLB Files (*.gltf *.glb)");
    filters.append("OSG Native Files (*.osg *.osgt *.osgb *.ive)");
    filters.append("Traditional 3D Models (*.obj *.3ds *.dae *.ply *.stl)");
    filters.append("Game Engine Formats (*.md2 *.md3 *.bsp)");
    filters.append("Professional Formats (*.fbx *.flt *.x)");
    filters.append("Geographic Formats (*.shp *.txp)");

    // 3. 添加"所有文件"选项
    filters.append("All Files (*.*)");

    return filters.join(";;");
}

// 统一的导出槽函数实现
void MainWindow::onExport()
{
    if (!osgContainer || !osgContainer->getRoot() || osgContainer->getRoot()->getNumChildren() == 0)
    {
        QMessageBox::warning(this, "导出失败", "没有可导出的模型");
        return;
    }

    // 构建文件过滤器，支持多种导出格式
    QString filter = 
        "OBJ Files (*.obj);;"
        "PLY Files (*.ply);;"
        "STL Files (*.stl);;"
        "OSG Text Files (*.osg *.osgt);;"
        "OSG Binary Files (*.osgb *.ive);;"
        "DAE Files (*.dae);;"
        "All Files (*.*)";

    QString fileName = QFileDialog::getSaveFileName(
        this,
        "导出模型",
        "",
        filter);

    if (!fileName.isEmpty())
    {
        // 确定文件格式
        QString selectedFilter = fileName; // 保存对话框返回的完整路径
        QString extension;
        
        // 从文件名中提取扩展名
        QFileInfo fileInfo(fileName);
        extension = fileInfo.suffix().toLower();
        
        // 如果没有扩展名，根据选择的过滤器添加
        if (extension.isEmpty())
        {
            // 这里需要获取用户选择的过滤器，但Qt的getSaveFileName不直接返回
            // 所以我们默认使用obj格式
            fileName += ".obj";
            extension = "obj";
        }
        
        // 根据扩展名执行相应的导出操作
        bool success = false;
        QString formatName;
        
        if (extension == "obj")
        {
            formatName = "OBJ";
            std::string filePath = StringUtil::QStringToLocalPath(fileName);
            success = osgDB::writeNodeFile(*osgContainer->getRoot(), filePath);
        }
        else if (extension == "ply")
        {
            formatName = "PLY";
            std::string filePath = StringUtil::QStringToLocalPath(fileName);
            success = osgDB::writeNodeFile(*osgContainer->getRoot(), filePath);
        }
        else if (extension == "stl")
        {
            formatName = "STL";
            std::string filePath = StringUtil::QStringToLocalPath(fileName);
            success = osgDB::writeNodeFile(*osgContainer->getRoot(), filePath);
        }
        else if (extension == "osg" || extension == "osgt")
        {
            formatName = (extension == "osg") ? "OSG ASCII" : "OSG Text";
            std::string filePath = StringUtil::QStringToLocalPath(fileName);
            success = osgDB::writeNodeFile(*osgContainer->getRoot(), filePath);
        }
        else if (extension == "osgb" || extension == "ive")
        {
            if (extension == "osgb")
            {
                formatName = "OSGB";
                success = osgContainer->exportToOSGB(fileName);
            }
            else
            {
                formatName = "IVE";
                std::string filePath = StringUtil::QStringToLocalPath(fileName);
                success = osgDB::writeNodeFile(*osgContainer->getRoot(), filePath);
            }
        }
        else if (extension == "dae")
        {
            formatName = "COLLADA";
            std::string filePath = StringUtil::QStringToLocalPath(fileName);
            success = osgDB::writeNodeFile(*osgContainer->getRoot(), filePath);
        }
        else if (extension == "fbx" || extension == "stp" || extension == "igs" || extension == "iges")
        {
            // 显示插件缺失的错误信息
            QString pluginName;
            if (extension == "fbx") pluginName = "FBX";
            else if (extension == "stp") pluginName = "STEP";
            else if (extension == "igs" || extension == "iges") pluginName = "IGES/IGS";
            
            QMessageBox::warning(this, "格式支持缺失", 
                QString("当前OSG版本缺少%1格式插件支持。\n\n"
                       "解决方案：\n"
                       "1. 升级到包含完整插件的OSG版本\n"
                       "2. 将文件转换为OBJ、PLY或STL格式\n"
                       "3. 检查OSG安装目录中的插件文件").arg(pluginName));
            return;
        }
        else
        {
            QMessageBox::warning(this, "导出失败", QString("不支持的文件格式: %1").arg(extension));
            return;
        }
        
        // 显示导出结果
        if (success)
        {
            QMessageBox::information(this, "导出成功", QString("模型已成功导出为 %1 格式:\n%2").arg(formatName).arg(fileName));
            statusBar()->showMessage(QString("%1 导出成功").arg(formatName), 3000);
        }
        else
        {
            QMessageBox::warning(this, "导出失败", QString("无法导出 %1 文件，请检查:\n• 文件路径和权限\n• 是否有相应的OSG写入插件\n• 模型数据是否有效").arg(formatName));
            statusBar()->showMessage(QString("%1 导出失败").arg(formatName), 3000);
        }
    }
}
