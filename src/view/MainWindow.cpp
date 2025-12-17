#include "MainWindow.h"
#include "OSGWidget.h"
#include <QMenuBar>
#include <QFileDialog>
#include <QStatusBar>
#include <functional>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    auto* w = new OSGWidget(this);
    setCentralWidget(w);
    resize(1024, 768);

    _statsLabel = new QLabel(this);
    statusBar()->addPermanentWidget(_statsLabel, 1);
    connect(w, &OSGWidget::statsUpdated, this, [this](double fps, double memMB) {
        _statsLabel->setText(QString("帧率: %1  内存: %2 MB").arg(fps, 0, 'f', 1).arg(memMB, 0, 'f', 1));
    });

    _treeDock = new QDockWidget(QString("模型结构"), this);
    _treeView = new QTreeView(_treeDock);
    _treeModel = new QStandardItemModel(_treeView);
    _treeModel->setHorizontalHeaderLabels(QStringList() << QString("节点"));
    _treeView->setModel(_treeModel);
    _treeDock->setWidget(_treeView);
    addDockWidget(Qt::LeftDockWidgetArea, _treeDock);

    _propDock = new QDockWidget(QString("属性"), this);
    _propView = new QTextEdit(_propDock);
    _propView->setReadOnly(true);
    _propDock->setWidget(_propView);
    addDockWidget(Qt::RightDockWidgetArea, _propDock);

    connect(w, &OSGWidget::propertiesUpdated, this, [this](const QString& text){
        _propView->setPlainText(text);
    });

    auto* fileMenu = menuBar()->addMenu(QString("文件"));
    auto* openAct = fileMenu->addAction(QString("打开..."));
    connect(openAct, &QAction::triggered, this, [this]() {
        QString filters = QString("3D 模型文件 (*.osg *.osgt *.ive *.obj *.gltf *.glb *.lmb);;所有文件 (*.*)");
        QString path = QFileDialog::getOpenFileName(this, QString("打开模型"), QString(), filters);
        if (path.isEmpty()) return;
        if (auto* v = qobject_cast<OSGWidget*>(centralWidget())) {
            if (v->loadModel(path)) {
                buildTree(v->currentNode());
            }
        }
    });
}

MainWindow::~MainWindow() {}

void MainWindow::buildTree(osg::Node* node) {
    _treeModel->removeRows(0, _treeModel->rowCount());
    if (!node) return;
    std::function<QStandardItem*(osg::Node*)> makeItem = [&](osg::Node* n) -> QStandardItem* {
        auto* item = new QStandardItem(QString::fromStdString(n->getName().empty() ? std::string(n->className()) : n->getName()));
        auto* group = n->asGroup();
        if (group) {
            for (unsigned int i = 0; i < group->getNumChildren(); ++i) {
                QStandardItem* childItem = makeItem(group->getChild(i));
                if (childItem) item->appendRow(childItem);
            }
        }
        return item;
    };
    QStandardItem* rootItem = makeItem(node);
    if (rootItem) _treeModel->appendRow(rootItem);
    _treeView->expandAll();
}
