#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QTreeView>
#include <QStandardItemModel>
#include <QDockWidget>
#include <osg/Node>
#include <QTextEdit>

class OSGWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    QLabel* _statsLabel = nullptr;
    QDockWidget* _treeDock = nullptr;
    QTreeView* _treeView = nullptr;
    QStandardItemModel* _treeModel = nullptr;
    QDockWidget* _propDock = nullptr;
    QTextEdit* _propView = nullptr;
    void buildTree(osg::Node* node);
};
