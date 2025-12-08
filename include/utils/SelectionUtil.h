#ifndef SELECTION_UTIL_H
#define SELECTION_UTIL_H

#include <vector>
#include <QRect>
#include <osg/Node>
#include <osg/Group>
#include <osgViewer/Viewer>

// 选择工具：封装点选/框选的 OSG 射线与多边形体检测逻辑
class SelectionUtil
{
public:
    // 点选：输入窗口坐标（Qt坐标系，原点左上），返回命中的最前节点（可为空）
    static osg::Node *PointPick(osgViewer::Viewer *viewer, int qtX, int qtY, int widgetHeight);

    // 框选：输入窗口矩形（Qt坐标系），返回命中的所有交点对应的末端节点（可能为空）
    static std::vector<osg::Node *> BoxSelect(osgViewer::Viewer *viewer, const QRect &qtRect, int widgetHeight);
};

#endif // SELECTION_UTIL_H
