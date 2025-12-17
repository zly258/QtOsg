#include <QApplication>
#include "view/MainWindow.h"
#include <osgDB/Registry>

int main(int argc, char *argv[]) {
    osgDB::Registry::instance()->loadLibrary("osgdb_lmb");
    osgDB::Registry::instance()->loadLibrary("osgdb_gltf");
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
