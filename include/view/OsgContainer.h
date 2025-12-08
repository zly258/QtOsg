#ifndef OSGCONTAINER_H
#define OSGCONTAINER_H

#include <QInputEvent>
#include <QOpenGLWidget>
#include <QObject>
#include <osgViewer/Viewer>
#include <osg/Group>
#include <osg/BoundingBox>
#include <osg/Vec3d>
#include <osg/StateSet>
#include <osg/observer_ptr>
#include <map>
#include <vector>
#include <QPoint>
#include <QDateTime>
#include <cmath>

// 高亮模式枚举
enum class HighlightMode {
    YellowGlow,      // 黄色发光（默认）
    RedPulse,        // 红色脉冲
    BlueGlow,        // 蓝色发光
    GreenGlow,       // 绿色发光
    OrangeGlow,      // 橙色发光
    CyanGlow,        // 青色发光
    PurpleGlow,      // 紫色发光
    WhiteGlow,       // 白色发光
    Wireframe        // 线框模式
};

class OsgContainer : public QOpenGLWidget, public osgViewer::Viewer
{
    Q_OBJECT
public:
    OsgContainer(QWidget *parent = 0);
    ~OsgContainer();

    std::map<unsigned int, int> m_key_map;

    int convertQKeyEnventToOSG(QKeyEvent *event);

    bool event(QEvent *event);

    void setKeyboardModifiers(QInputEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void resizeEvent(QResizeEvent *event);
    void moveEvent(QMoveEvent *event);

    osgViewer::Viewer *getOSGViewer()
    {
        return this;
    }
    osg::Group *getRoot()
    {
        return root;
    }

    // 视图控制（公开接口，供外部调用）
    void fitToView();
    void fitToNodes(const std::vector<osg::Node *> &nodes);
    void setViewDirection(const osg::Vec3d &dir, const osg::Vec3d &up);
    void viewFront();
    void viewBack();
    void viewLeft();
    void viewRight();
    void viewTop();
    void viewBottom();
    void viewIsoNE();
    void viewIsoNW();
    void viewIsoSE();
    void viewIsoSW();

    // 高亮
    void highlightNode(osg::Node *node);
    void highlightNodes(const std::vector<osg::Node *> &nodes);
    void clearHighlight();
    
    // 高亮模式控制
    void setHighlightMode(HighlightMode mode);
    HighlightMode getHighlightMode() const { return m_highlightMode; }
    
    // 设置高亮强度（0.0-2.0）
    void setHighlightIntensity(float intensity) { m_highlightIntensity = intensity; }
    float getHighlightIntensity() const { return m_highlightIntensity; }
    
    // 启用/禁用高亮动画
    void setHighlightAnimationEnabled(bool enabled) { m_highlightAnimationEnabled = enabled; }
    bool isHighlightAnimationEnabled() const { return m_highlightAnimationEnabled; }
    
    // 性能优化控制
    enum class PerformanceMode {
        HighQuality,    // 高质量：完整效果，性能较低
        Balanced,       // 平衡模式：适中效果和性能
        HighPerformance // 高性能：简化效果，性能优先
    };
    
    void setPerformanceMode(PerformanceMode mode) { m_performanceMode = mode; }
    PerformanceMode getPerformanceMode() const { return m_performanceMode; }
    
    // 设置最大高亮节点数限制（0表示无限制）
    void setMaxHighlightedNodes(int maxNodes) { m_maxHighlightedNodes = maxNodes; }
    int getMaxHighlightedNodes() const { return m_maxHighlightedNodes; }
    
    // 设置动画帧率限制
    void setMaxAnimationFPS(int fps) { m_maxAnimationFPS = fps; }
    int getMaxAnimationFPS() const { return m_maxAnimationFPS; }
    
    // 动画控制
    enum class AnimationType {
        None,        // 无动画
        Pulse,       // 脉冲动画（亮度周期性变化）
        Blink,       // 闪烁动画（快速开关）
        Gradient     // 渐变动画（颜色渐变）
    };
    
    void setAnimationType(AnimationType type) { m_animationType = type; }
    AnimationType getAnimationType() const { return m_animationType; }
    
    // 设置动画速度（0.1-5.0，1.0为正常速度）
    void setAnimationSpeed(float speed) { m_animationSpeed = speed; }
    float getAnimationSpeed() const { return m_animationSpeed; }
    
    // 重启动画
    void restartAnimation() { m_animationStartTime = QDateTime::currentMSecsSinceEpoch(); }

    // 导出：将当前场景导出为 .osgb（二进制），返回是否成功（仅 OSG 模式有效）
    bool exportToOSGB(const std::string &filePath) const;
    bool exportToOSGB(const QString &filePath) const;

    // 单选开关
    void setSinglePickEnabled(bool on) { m_singlePickEnabled = on; }
    bool isSinglePickEnabled() const { return m_singlePickEnabled; }

    // 视图辅助：充满视图（对整棵场景）
    // 视图辅助：显示/隐藏场景包围盒
    void setShowSceneBBox(bool on);
    bool isSceneBBoxShown() const { return m_showBBox; }

protected:
    virtual void paintGL();

private:
    void init3D();
    osg::ref_ptr<osg::Camera> createCamera(int x, int y, int w, int h);
    // 基于窗口坐标进行射线拾取，返回命中的节点（优先返回可拾取的 Geode）
    osg::Node *pickNodeAt(int x, int y) const;

private:
    osg::ref_ptr<osg::Group> root;
    osgViewer::GraphicsWindow *window;
    // 拾取窗口坐标的世界点（用于设置旋转中心）
    bool pickAt(int x, int y, osg::Vec3d &world) const;
    // OsgContainer.cpp 中使用的私有工具函数声明
    bool computeViewSpaceHalfExtent(const osg::BoundingBox &bb, double &halfW, double &halfH) const;
    void adjustOrthoByAspect();
    osg::ref_ptr<osg::Geode> buildBBoxGeode(const osg::BoundingBox &bb) const;

    // 高亮相关
    std::vector<osg::observer_ptr<osg::Node>> m_highlighted;
    std::map<osg::observer_ptr<osg::Node>, osg::ref_ptr<osg::StateSet>> m_savedStateSet;
    osg::ref_ptr<osg::StateSet> createHighlightStateSet(HighlightMode mode = HighlightMode::YellowGlow);
    osg::ref_ptr<osg::StateSet> createOptimizedHighlightStateSet(HighlightMode mode);
    osg::ref_ptr<osg::StateSet> createSimpleHighlightStateSet(HighlightMode mode);
    osg::ref_ptr<osg::StateSet> createBalancedHighlightStateSet(HighlightMode mode);
    
    // 动画相关函数
    void applyAnimationEffects(osg::StateSet* ss, HighlightMode mode);
    void updateAnimationEffects();
    
    // 高亮配置
    HighlightMode m_highlightMode = HighlightMode::YellowGlow;
    float m_highlightIntensity = 1.0f;
    bool m_highlightAnimationEnabled = true;
    
    // 动画配置
    AnimationType m_animationType = AnimationType::None;
    float m_animationSpeed = 1.0f;
    qint64 m_animationStartTime = 0;
    
    // 性能优化配置
    PerformanceMode m_performanceMode = PerformanceMode::Balanced;
    int m_maxHighlightedNodes = 0;        // 0表示无限制
    int m_maxAnimationFPS = 60;           // 默认60FPS
    qint64 m_lastAnimationUpdate = 0;     // 上次动画更新时间
    int m_currentHighlightedCount = 0;    // 当前高亮节点计数

    // 单选与交互相关
    bool m_singlePickEnabled = false;        // 是否启用单选
    osg::observer_ptr<osg::Node> m_selected; // 当前选中的节点

    // 鼠标交互状态
    bool m_leftButtonDown = false;
    bool m_rightButtonDown = false;
    QPoint m_lastMousePos;
    // 使用OSG默认交互，不做额外平移系数

    // 场景包围盒显示
    bool m_showBBox = false;
    osg::ref_ptr<osg::Geode> m_bboxGeode;

    // 持续渲染定时器
    class QTimer *m_renderTimer = nullptr;

    // 光照设置
    void setupUniformLighting();

signals:
    // 选中/清空信号，供 MainWindow 同步树与状态
    void nodePicked(osg::Node *node);
    void selectionCleared();
};

#endif // OSGCONTAINER_H
