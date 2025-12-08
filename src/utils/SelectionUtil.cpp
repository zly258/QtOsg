#include "utils/SelectionUtil.h"
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/PolytopeIntersector>
#include <osgUtil/IntersectionVisitor>
#include <algorithm>
#include <osg/Geode>
#include <unordered_set>

// 拾取掩码（如需按掩码限定可拾取对象，可启用）；当前不再用来限制遍历
static const unsigned int NODE_PICK = 0x0Fu;   // 可拾取（保留定义）
static const unsigned int NODE_NPICK = ~0x0Fu; // 不可拾取（保留定义）

osg::Node *SelectionUtil::PointPick(osgViewer::Viewer *viewer, int qtX, int qtY, int widgetHeight)
{
    if (!viewer)
        return nullptr;
    // 射线拾取：使用 viewer->computeIntersections（内部用 LineSegmentIntersector）
    const double x = static_cast<double>(qtX);
    const double y = static_cast<double>(widgetHeight - qtY);
    osgUtil::LineSegmentIntersector::Intersections hits;
    // 不使用限制掩码，避免父节点被屏蔽导致无法遍历到子 Geode
    if (viewer->computeIntersections(x, y, hits))
    {
        const auto &hit = *hits.begin(); // 最近命中
        // 优先返回 Geode，以便与树“网格名称”一致
        for (auto it = hit.nodePath.rbegin(); it != hit.nodePath.rend(); ++it)
        {
            osg::Node *n = *it;
            if (dynamic_cast<osg::Geode *>(n))
                return n;
        }
        // 若路径中没有 Geode，则返回第一个命中的节点
        for (auto it = hit.nodePath.rbegin(); it != hit.nodePath.rend(); ++it)
        {
            if (*it)
                return *it;
        }
    }
    return nullptr;
}

std::vector<osg::Node *> SelectionUtil::BoxSelect(osgViewer::Viewer *viewer, const QRect &qtRect, int widgetHeight)
{
    std::vector<osg::Node *> result;
    if (!viewer || qtRect.isEmpty())
        return result;

    // 统一使用射线网格采样（例如步长5像素），避免大多边形求交导致的卡顿
    const int xMinQt = std::min(qtRect.left(), qtRect.right());
    const int xMaxQt = std::max(qtRect.left(), qtRect.right());
    const int yMinQt = std::min(qtRect.top(), qtRect.bottom());
    const int yMaxQt = std::max(qtRect.top(), qtRect.bottom());
    const int STEP = 5; // 采样步长（像素）

    std::unordered_set<osg::Node *> uniq;
    osgUtil::LineSegmentIntersector::Intersections hits;
    for (int xq = xMinQt; xq <= xMaxQt; xq += STEP)
    {
        for (int yq = yMinQt; yq <= yMaxQt; yq += STEP)
        {
            const double x = static_cast<double>(xq);
            const double y = static_cast<double>(widgetHeight - yq);
            hits.clear();
            if (viewer->computeIntersections(x, y, hits))
            {
                const auto &hit = *hits.begin();
                // 优先收集 Geode
                osg::Node *chosen = nullptr;
                for (auto it = hit.nodePath.rbegin(); it != hit.nodePath.rend(); ++it)
                {
                    osg::Node *n = *it;
                    if (dynamic_cast<osg::Geode *>(n))
                    {
                        chosen = n;
                        break;
                    }
                }
                if (!chosen && !hit.nodePath.empty())
                    chosen = hit.nodePath.back();
                if (chosen)
                    uniq.insert(chosen);
            }
        }
    }

    result.reserve(uniq.size());
    for (auto *n : uniq)
        result.push_back(n);
    return result;
}
