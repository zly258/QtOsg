#include "utils/SceneStructureParser.h"
#include <osg/Geometry>
#include <osg/PrimitiveSet>
#include <osg/NodeVisitor>
#include <QDebug>

SceneStructureParser::SceneStructureParser()
{
}

SceneStructureParser::~SceneStructureParser()
{
}

void SceneStructureParser::parseAndBuildTree(osg::Node* rootNode, QTreeWidget* treeWidget)
{
    if (!rootNode || !treeWidget)
        return;

    // 清空现有树结构
    treeWidget->clear();

    // 创建根节点项
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(treeWidget);
    rootItem->setText(0, getNodeDisplayName(rootNode));
    rootItem->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void*>(rootNode)));
    rootItem->setIcon(0, QIcon(":/icons/scene_root.svg"));
    rootItem->setExpanded(true);
    
    // 设置节点掩码以支持拾取
    rootNode->setNodeMask(0xFFu);

    // 解析子节点
    parseNodeRecursive(rootNode, rootItem);
}

void SceneStructureParser::parseNodeRecursive(osg::Node* node, QTreeWidgetItem* parentItem)
{
    if (!node || !parentItem)
        return;

    // 如果是Group节点，递归处理子节点
    osg::Group* group = node->asGroup();
    if (group)
    {
        for (unsigned int i = 0; i < group->getNumChildren(); ++i)
        {
            osg::Node* childNode = group->getChild(i);
            if (childNode)
            {
                QTreeWidgetItem* childItem = new QTreeWidgetItem(parentItem);
                childItem->setText(0, getNodeDisplayName(childNode));
                childItem->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void*>(childNode)));
                childItem->setIcon(0, QIcon(QString(":/icons/%1.svg").arg(getNodeIconType(childNode))));
                childItem->setToolTip(0, getNodeStatistics(childNode));
                
                // 设置节点掩码以支持拾取
                childNode->setNodeMask(0xFFu);
                
                // 递归处理子节点
                parseNodeRecursive(childNode, childItem);
            }
        }
    }
}

QString SceneStructureParser::getNodeDisplayName(osg::Node* node)
{
    if (!node)
        return "Null Node";

    // 获取节点名称
    std::string nodeName = node->getName();
    if (!nodeName.empty())
    {
        return QString::fromStdString(nodeName);
    }

    // 根据节点类型生成默认名称
    QString className = QString::fromStdString(node->className());
    
    // 如果是Geode，显示几何体数量
    osg::Geode* geode = node->asGeode();
    if (geode)
    {
        int geodeCount = geode->getNumDrawables();
        if (geodeCount == 1)
            return QString("%1 (1 geometry)").arg(className);
        else
            return QString("%1 (%2 geometries)").arg(className).arg(geodeCount);
    }

    // 如果是Group，显示子节点数量
    osg::Group* group = node->asGroup();
    if (group)
    {
        int childCount = group->getNumChildren();
        if (childCount == 0)
            return QString("%1 (empty)").arg(className);
        else if (childCount == 1)
            return QString("%1 (1 child)").arg(className);
        else
            return QString("%1 (%2 children)").arg(className).arg(childCount);
    }

    return className;
}

QString SceneStructureParser::getNodeIconType(osg::Node* node)
{
    if (!node)
        return "unknown";

    // 根据节点类型返回对应的图标
    osg::Geode* geode = node->asGeode();
    if (geode)
    {
        int drawableCount = geode->getNumDrawables();
        if (drawableCount == 1)
            return "geometry_single";
        else if (drawableCount > 1)
            return "geometry_multiple";
        return "geode";
    }

    osg::Group* group = node->asGroup();
    if (group)
    {
        int childCount = group->getNumChildren();
        if (childCount == 0)
            return "group_empty";
        else if (childCount <= 5)
            return "group_small";
        else if (childCount <= 20)
            return "group_medium";
        else
            return "group_large";
    }

    // MatrixTransform - 检查节点类型名称
    if (node->className() == std::string("MatrixTransform"))
        return "transform";

    // 其他节点类型
    return "node";
}

QString SceneStructureParser::getNodeStatistics(osg::Node* node)
{
    if (!node)
        return "Null node";

    QString stats;
    
    // 基本信息
    stats += QString("类型: %1\n").arg(QString::fromStdString(node->className()));
    
    // 节点名称
    std::string nodeName = node->getName();
    if (!nodeName.empty())
    {
        stats += QString("名称: %1\n").arg(QString::fromStdString(nodeName));
    }
    
    // 节点掩码
    stats += QString("节点掩码: 0x%1\n").arg(node->getNodeMask(), 0, 16);
    
    // 几何体统计
    int geodeCount = countGeodesRecursive(node);
    if (geodeCount > 0)
    {
        stats += QString("几何体: %1 个\n").arg(geodeCount);
        
        int triangleCount = countTrianglesRecursive(node);
        int vertexCount = countVerticesRecursive(node);
        
        if (triangleCount > 0)
            stats += QString("三角形: %1 个\n").arg(triangleCount);
        if (vertexCount > 0)
            stats += QString("顶点: %1 个\n").arg(vertexCount);
    }
    
    // 包围盒信息
    osg::BoundingSphere bs = node->getBound();
    if (bs.valid())
    {
        stats += QString("包围球:\n");
        stats += QString("  中心: (%.2f, %.2f, %.2f)\n").arg(bs.center().x()).arg(bs.center().y()).arg(bs.center().z());
        stats += QString("  半径: %.2f\n").arg(bs.radius());
    }
    
    return stats;
}

int SceneStructureParser::countGeodesRecursive(osg::Node* node)
{
    if (!node)
        return 0;

    int count = 0;
    osg::Geode* geode = node->asGeode();
    if (geode)
        count = 1;

    osg::Group* group = node->asGroup();
    if (group)
    {
        for (unsigned int i = 0; i < group->getNumChildren(); ++i)
        {
            count += countGeodesRecursive(group->getChild(i));
        }
    }

    return count;
}

int SceneStructureParser::countTrianglesRecursive(osg::Node* node)
{
    if (!node)
        return 0;

    int count = 0;
    osg::Geode* geode = node->asGeode();
    if (geode)
    {
        for (unsigned int i = 0; i < geode->getNumDrawables(); ++i)
        {
            osg::Geometry* geometry = geode->getDrawable(i)->asGeometry();
            if (geometry)
            {
                // 使用兼容的API获取图元信息
                osg::Geometry::PrimitiveSetList& primitives = geometry->getPrimitiveSetList();
                for (auto& primitive : primitives)
                {
                    // 对于较老版本的OSG，简化图元计数逻辑
                    if (primitive->getNumIndices() >= 3)
                    {
                        // 假设大部分图元都是三角形，进行粗略估计
                        count += primitive->getNumIndices() / 3;
                    }
                }
            }
        }
    }

    osg::Group* group = node->asGroup();
    if (group)
    {
        for (unsigned int i = 0; i < group->getNumChildren(); ++i)
        {
            count += countTrianglesRecursive(group->getChild(i));
        }
    }

    return count;
}

int SceneStructureParser::countVerticesRecursive(osg::Node* node)
{
    if (!node)
        return 0;

    int count = 0;
    osg::Geode* geode = node->asGeode();
    if (geode)
    {
        for (unsigned int i = 0; i < geode->getNumDrawables(); ++i)
        {
            osg::Geometry* geometry = geode->getDrawable(i)->asGeometry();
            if (geometry && geometry->getVertexArray())
            {
                count += geometry->getVertexArray()->getNumElements();
            }
        }
    }

    osg::Group* group = node->asGroup();
    if (group)
    {
        for (unsigned int i = 0; i < group->getNumChildren(); ++i)
        {
            count += countVerticesRecursive(group->getChild(i));
        }
    }

    return count;
}