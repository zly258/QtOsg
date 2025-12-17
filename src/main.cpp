#include <QApplication>
#include <QFont>
#include "view/MainWindow.h"
#include <osgDB/Registry>

int main(int argc, char *argv[]) {
    osgDB::Registry::instance()->loadLibrary("osgdb_lmb");
    osgDB::Registry::instance()->loadLibrary("osgdb_gltf");
    QApplication app(argc, argv);
    app.setFont(QFont("Microsoft YaHei", 10));
    MainWindow w;
    w.show();
    return app.exec();
}
