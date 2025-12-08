#include <QApplication>
#include <QFont>
#include <QDebug>
#include <QStyleFactory>
#include <QPalette>
#include <QIcon>
#include <QTranslator>
#include "view/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 全局关闭窗口标题栏的“？”帮助按钮
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton, true);

    // 使用 Fusion 风格
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    app.setApplicationName("轻量化模型浏览器");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("LMBViewer");

    // 设置全局字体为微软雅黑
    QFont font("Microsoft YaHei", 9);
    app.setFont(font);

    // 统一聚焦与非聚焦的高亮颜色
    QPalette pal = app.palette();
    const QColor hl = pal.color(QPalette::Active, QPalette::Highlight);
    const QColor hltxt = pal.color(QPalette::Active, QPalette::HighlightedText);
    for (QPalette::ColorGroup grp : {QPalette::Active, QPalette::Inactive, QPalette::Disabled})
    {
        pal.setColor(grp, QPalette::Highlight, hl);
        pal.setColor(grp, QPalette::HighlightedText, hltxt);
    }
    app.setPalette(pal);

    // 安装中文翻译（从资源加载）
    static QTranslator qtTr;
    qtTr.load(":/i18n/qt_zh_CN.qm");
    static QTranslator qtbaseTr;
    qtbaseTr.load(":/i18n/qtbase_zh_CN.qm");
    app.installTranslator(&qtTr);
    app.installTranslator(&qtbaseTr);

    qDebug() << "Starting LMB Model Viewer...";

    // 设置应用与主窗口图标（来自资源文件）
    app.setWindowIcon(QIcon(":/icons/app_icon.svg"));

    MainWindow window;
    window.setWindowIcon(QIcon(":/icons/app_icon.svg"));
    window.show();

    return app.exec();
}