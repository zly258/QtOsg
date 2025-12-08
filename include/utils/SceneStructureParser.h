#ifndef SCENE_STRUCTURE_PARSER_H
#define SCENE_STRUCTURE_PARSER_H

#include <osg/Node>
#include <osg/Group>
#include <osg/Geode>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QString>

/**
 * @brief 场景结构解析器
 * 
 * 用于解析OSG场景图的层级结构，并在Qt树形控件中显示
 */
class SceneStructureParser
{
public:
    SceneStructureParser();
    ~SceneStructureParser();

    /**
     * @brief 解析节点并构建树结构
     * @param rootNode OSG根节点
     * @param treeWidget 目标树形控件
     */
    void parseAndBuildTree(osg::Node* rootNode, QTreeWidget* treeWidget);

    /**
     * @brief 解析单个节点及其子节点
     * @param node OSG节点
     * @param parentItem 父树项
     */
    void parseNodeRecursive(osg::Node* node, QTreeWidgetItem* parentItem);

    /**
     * @brief 获取节点显示名称
     * @param node OSG节点
     * @return 显示名称
     */
    QString getNodeDisplayName(osg::Node* node);

    /**
     * @brief 获取节点图标类型
     * @param node OSG节点
     * @return 图标类型字符串
     */
    QString getNodeIconType(osg::Node* node);

    /**
     * @brief 获取节点统计信息
     * @param node OSG节点
     * @return 统计信息字符串
     */
    QString getNodeStatistics(osg::Node* node);

private:
    /**
     * @brief 递归计算几何体数量
     * @param node OSG节点
     * @return 几何体数量
     */
    int countGeodesRecursive(osg::Node* node);

    /**
     * @brief 递归计算三角形数量
     * @param node OSG节点
     * @return 三角形数量
     */
    int countTrianglesRecursive(osg::Node* node);

    /**
     * @brief 递归计算顶点数量
     * @param node OSG节点
     * @return 顶点数量
     */
    int countVerticesRecursive(osg::Node* node);
};

#endif // SCENE_STRUCTURE_PARSER_H