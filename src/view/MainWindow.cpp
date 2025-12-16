#include "MainWindow.h"
#include "OSGWidget.h"
#include <QMenuBar>
#include <QFileDialog>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    auto* w = new OSGWidget(this);
    setCentralWidget(w);
    resize(1024, 768);
    auto* fileMenu = menuBar()->addMenu(QString("文件"));
    auto* openAct = fileMenu->addAction(QString("打开..."));
    connect(openAct, &QAction::triggered, this, [this]() {
        QString filters = QString("3D Files (*.osg *.osgt *.ive *.obj *.gltf *.glb *.lmb);;All Files (*.*)");
        QString path = QFileDialog::getOpenFileName(this, QString("打开模型"), QString(), filters);
        if (path.isEmpty()) return;
        if (auto* v = qobject_cast<OSGWidget*>(centralWidget())) {
            v->loadModel(path);
        }
    });
}

MainWindow::~MainWindow() {}
