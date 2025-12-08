#include "view/OsgContainer.h"
#include <QInputEvent>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/MultiTouchTrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/ViewerEventHandlers>
#include <QApplication>
#include <osg/LightModel>
#include <osg/CullFace>
#include <osg/CullSettings>
#include <osg/Material>
#include <osg/Multisample>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/ComputeBoundsVisitor>
#include <osg/BoundingSphere>
#include <osg/Material>
#include <osgText/Text>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/LineWidth>
#include <osg/NodeVisitor>

#include "qdebug.h"
#include "QOpenGLContext.h"
#include "utils/StringUtil.h"
#include <QElapsedTimer>
#include <QTimer>
#include <QDateTime>
#include <algorithm>
#include <osgDB/DatabasePager>

OsgContainer::OsgContainer(QWidget *parent)
    : QOpenGLWidget(parent)
{

    init3D();
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    QSurfaceFormat surfaceForamt;
    // 使用兼容模式（Compatibility Profile），避免 OSG 3.6.5 固定管线状态在 Core Profile 下产生 GL 错误
    surfaceForamt.setRenderableType(QSurfaceFormat::OpenGL);
    surfaceForamt.setProfile(QSurfaceFormat::CompatibilityProfile);
    surfaceForamt.setVersion(3, 3); // OpenGL 3.3 Compatibility
    surfaceForamt.setDepthBufferSize(24);
    surfaceForamt.setStencilBufferSize(8);
    surfaceForamt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    surfaceForamt.setSamples(4); // MSAA 4x
    setFormat(surfaceForamt);

    m_key_map[Qt::Key_Escape] = osgGA::GUIEventAdapter::KEY_Escape;
    m_key_map[Qt::Key_Delete] = osgGA::GUIEventAdapter::KEY_Delete;
    m_key_map[Qt::Key_Home] = osgGA::GUIEventAdapter::KEY_Home;
    m_key_map[Qt::Key_Enter] = osgGA::GUIEventAdapter::KEY_KP_Enter;
    m_key_map[Qt::Key_End] = osgGA::GUIEventAdapter::KEY_End;
    m_key_map[Qt::Key_Return] = osgGA::GUIEventAdapter::KEY_Return;
    m_key_map[Qt::Key_PageUp] = osgGA::GUIEventAdapter::KEY_Page_Up;
    m_key_map[Qt::Key_PageDown] = osgGA::GUIEventAdapter::KEY_Page_Down;
    m_key_map[Qt::Key_Left] = osgGA::GUIEventAdapter::KEY_Left;
    m_key_map[Qt::Key_Right] = osgGA::GUIEventAdapter::KEY_Right;
    m_key_map[Qt::Key_Up] = osgGA::GUIEventAdapter::KEY_Up;
    m_key_map[Qt::Key_Down] = osgGA::GUIEventAdapter::KEY_Down;
    m_key_map[Qt::Key_Backspace] = osgGA::GUIEventAdapter::KEY_BackSpace;
    m_key_map[Qt::Key_Tab] = osgGA::GUIEventAdapter::KEY_Tab;
    m_key_map[Qt::Key_Space] = osgGA::GUIEventAdapter::KEY_Space;
    m_key_map[Qt::Key_Delete] = osgGA::GUIEventAdapter::KEY_Delete;
    m_key_map[Qt::Key_Alt] = osgGA::GUIEventAdapter::KEY_Alt_L;
    m_key_map[Qt::Key_Shift] = osgGA::GUIEventAdapter::KEY_Shift_L;
    m_key_map[Qt::Key_Control] = osgGA::GUIEventAdapter::KEY_Control_L;
    m_key_map[Qt::Key_Meta] = osgGA::GUIEventAdapter::KEY_Meta_L;
    m_key_map[Qt::Key_F1] = osgGA::GUIEventAdapter::KEY_F1;
    m_key_map[Qt::Key_F2] = osgGA::GUIEventAdapter::KEY_F2;
    m_key_map[Qt::Key_F3] = osgGA::GUIEventAdapter::KEY_F3;
    m_key_map[Qt::Key_F4] = osgGA::GUIEventAdapter::KEY_F4;
    m_key_map[Qt::Key_F5] = osgGA::GUIEventAdapter::KEY_F5;
    m_key_map[Qt::Key_F6] = osgGA::GUIEventAdapter::KEY_F6;
    m_key_map[Qt::Key_F7] = osgGA::GUIEventAdapter::KEY_F7;
    m_key_map[Qt::Key_F8] = osgGA::GUIEventAdapter::KEY_F8;
    m_key_map[Qt::Key_F9] = osgGA::GUIEventAdapter::KEY_F9;
    m_key_map[Qt::Key_F10] = osgGA::GUIEventAdapter::KEY_F10;
    m_key_map[Qt::Key_F11] = osgGA::GUIEventAdapter::KEY_F11;
    m_key_map[Qt::Key_F12] = osgGA::GUIEventAdapter::KEY_F12;
    m_key_map[Qt::Key_F13] = osgGA::GUIEventAdapter::KEY_F13;
    m_key_map[Qt::Key_F14] = osgGA::GUIEventAdapter::KEY_F14;
    m_key_map[Qt::Key_F15] = osgGA::GUIEventAdapter::KEY_F15;
    m_key_map[Qt::Key_F16] = osgGA::GUIEventAdapter::KEY_F16;
    m_key_map[Qt::Key_F17] = osgGA::GUIEventAdapter::KEY_F17;
    m_key_map[Qt::Key_F18] = osgGA::GUIEventAdapter::KEY_F18;
    m_key_map[Qt::Key_F19] = osgGA::GUIEventAdapter::KEY_F19;
    m_key_map[Qt::Key_F20] = osgGA::GUIEventAdapter::KEY_F20;
    m_key_map[Qt::Key_hyphen] = '-';
    m_key_map[Qt::Key_Equal] = '=';
    m_key_map[Qt::Key_division] = osgGA::GUIEventAdapter::KEY_KP_Divide;
    m_key_map[Qt::Key_multiply] = osgGA::GUIEventAdapter::KEY_KP_Multiply;
    m_key_map[Qt::Key_Minus] = '-';
    m_key_map[Qt::Key_Plus] = '+';
    m_key_map[Qt::Key_Insert] = osgGA::GUIEventAdapter::KEY_KP_Insert;

    for (uint i = Qt::Key_A; i < Qt::Key_Z; i++)
    {
        m_key_map[i] = (osgGA::GUIEventAdapter::KEY_A - Qt::Key_A) + i;
    }
}

// 导出：将当前场景导出为 .osgb（二进制）
bool OsgContainer::exportToOSGB(const std::string &filePath) const
{
    // 获取根节点并检查
    osg::Group *r = const_cast<OsgContainer *>(this)->getRoot();
    if (!r)
        return false;
    // 使用 OSG 的写文件接口导出为 .osgb
    return osgDB::writeNodeFile(*r, filePath);
}

bool OsgContainer::exportToOSGB(const QString &filePath) const
{
    // 使用本地编码支持中文路径
    std::string localPath = StringUtil::QStringToLocalPath(filePath);
    return exportToOSGB(localPath);
}

// 基于窗口坐标进行射线拾取，返回命中的节点（优先 Geode；受可拾取掩码影响）
osg::Node *OsgContainer::pickNodeAt(int x, int y) const
{
    osg::Camera *cam = const_cast<OsgContainer *>(this)->getCamera();
    if (!cam || !root.valid())
        return nullptr;
    osg::Viewport *vp = cam->getViewport();
    if (!vp)
        return nullptr;
    double wx = static_cast<double>(x);
    double wy = static_cast<double>(vp->height()) - 1.0 - static_cast<double>(y);

    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
        new osgUtil::LineSegmentIntersector(osgUtil::Intersector::WINDOW, wx, wy);
    osgUtil::IntersectionVisitor iv(intersector.get());
    // 遍历全部节点，命中后再按可拾取掩码(低4位)过滤
    cam->accept(iv);
    if (!intersector->containsIntersections())
        return nullptr;
    const auto &hits = intersector->getIntersections();
    for (const auto &hit : hits)
    {
        // 从命中路径中自底向上查找第一个可拾取的 Geode
        osg::NodePath np = hit.nodePath;
        for (auto rit = np.rbegin(); rit != np.rend(); ++rit)
        {
            osg::Node *n = *rit;
            if (auto *gd = dynamic_cast<osg::Geode *>(n))
            {
                if ((gd->getNodeMask() & 0x0Fu) != 0)
                    return gd;
            }
        }
        // 若无 Geode，则返回路径末尾节点（需判断可拾取位）
        if (!np.empty())
        {
            osg::Node *tail = np.back();
            if ((tail->getNodeMask() & 0x0Fu) != 0)
                return tail;
        }
    }
    return nullptr;
}

OsgContainer::~OsgContainer()
{
}

bool OsgContainer::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::TouchUpdate:
    {
        QList<QTouchEvent::TouchPoint> touchPoints = static_cast<QTouchEvent *>(event)->touchPoints();
        unsigned int id = 0;
        unsigned int tapCount = touchPoints.size();

        osg::ref_ptr<osgGA::GUIEventAdapter> osgEvent(NULL);
        osgGA::GUIEventAdapter::TouchPhase phase = osgGA::GUIEventAdapter::TOUCH_UNKNOWN;
        foreach (const QTouchEvent::TouchPoint &touchPoint, touchPoints)
        {
            if (!osgEvent)
            {
                if (event->type() == QEvent::TouchBegin)
                {
                    phase = osgGA::GUIEventAdapter::TOUCH_BEGAN;
                    osgEvent = window->getEventQueue()->touchBegan(id, osgGA::GUIEventAdapter::TOUCH_BEGAN, touchPoint.pos().x(), touchPoint.pos().y());
                }
                else if (event->type() == QEvent::TouchEnd)
                {
                    phase = osgGA::GUIEventAdapter::TOUCH_ENDED;
                    osgEvent = window->getEventQueue()->touchEnded(id, osgGA::GUIEventAdapter::TOUCH_ENDED, touchPoint.pos().x(), touchPoint.pos().y(), tapCount);
                }
                else if (event->type() == QEvent::TouchUpdate)
                {
                    phase = osgGA::GUIEventAdapter::TOUCH_MOVED;
                    osgEvent = window->getEventQueue()->touchMoved(id, osgGA::GUIEventAdapter::TOUCH_MOVED, touchPoint.pos().x(), touchPoint.pos().y());
                }
            }
            else
            {
                osgEvent->addTouchPoint(id, osgGA::GUIEventAdapter::TOUCH_ENDED, touchPoint.pos().x(), touchPoint.pos().y());
                osgEvent->addTouchPoint(id, phase, touchPoint.pos().x(), touchPoint.pos().y());
            }
            id++;
        }
        break;
    }
    default:
        break;
    }
    return QOpenGLWidget::event(event);
}

void OsgContainer::setKeyboardModifiers(QInputEvent *event)
{
    int modkey = event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
    unsigned int mask = 0;
    if (modkey & Qt::ShiftModifier)
    {
        mask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
    }
    if (modkey & Qt::ControlModifier)
    {
        mask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
    }
    if (modkey & Qt::AltModifier)
    {
        mask |= osgGA::GUIEventAdapter::MODKEY_ALT;
    }

    window->getEventQueue()->getCurrentEventState()->setModKeyMask(mask);
}

int OsgContainer::convertQKeyEnventToOSG(QKeyEvent *event)
{
    auto itr = m_key_map.find(event->key());
    if (itr == m_key_map.end())
    {
        return int(*(event->text().toLatin1().data()));
    }
    else
    {
        return itr->second;
    }
}

void OsgContainer::keyPressEvent(QKeyEvent *event)
{
    setKeyboardModifiers(event);
    int value = convertQKeyEnventToOSG(event);
    window->getEventQueue()->keyPress(value);
    QOpenGLWidget::keyPressEvent(event);
}

void OsgContainer::keyReleaseEvent(QKeyEvent *event)
{
    setKeyboardModifiers(event);
    window->getEventQueue()->keyRelease(event->key());
    QOpenGLWidget::keyReleaseEvent(event);
}

void OsgContainer::mousePressEvent(QMouseEvent *event)
{
    int button = 0;
    switch (event->button())
    {
    case Qt::LeftButton:
        button = 1;
        break;
    case Qt::MidButton:
        button = 2;
        break;
    case Qt::RightButton:
        button = 3;
        break;
    case Qt::NoButton:
        button = 0;
        break;
    default:
        button = 0;
        break;
    }
    setKeyboardModifiers(event);
    window->getEventQueue()->mouseButtonPress(event->x(), event->y(), button);
}

void OsgContainer::mouseReleaseEvent(QMouseEvent *event)
{
    int button = 0;
    switch (event->button())
    {
    case Qt::LeftButton:
        button = 1;
        break;
    case Qt::MidButton:
        button = 2;
        break;
    case Qt::RightButton:
        button = 3;
        break;
    case Qt::NoButton:
        button = 0;
        break;
    default:
        button = 0;
        break;
    }
    setKeyboardModifiers(event);
    window->getEventQueue()->mouseButtonRelease(event->x(), event->y(), button);

    QOpenGLWidget::mouseReleaseEvent(event);
    // 左键释放时做单选拾取
    if (m_singlePickEnabled && event->button() == Qt::LeftButton)
    {
        osg::Node *picked = pickNodeAt(event->x(), event->y());
        if (picked)
        {
            // 清旧高亮，设置新高亮
            clearHighlight();
            highlightNode(picked);
            m_selected = picked;
            emit nodePicked(picked);
        }
        else
        {
            // 空白区：清空选择
            m_selected = nullptr;
            clearHighlight();
            emit selectionCleared();
        }
    }
}

// 更新所有高亮节点的动画效果
void OsgContainer::updateAnimationEffects()
{
    if (m_animationType == AnimationType::None || !m_highlightAnimationEnabled)
        return;
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    float elapsed = (currentTime - m_animationStartTime) / 1000.0f * m_animationSpeed;
    
    // 为每个高亮的节点更新动画效果
    for (auto &kv : m_savedStateSet)
    {
        osg::Node* node = kv.first.get();
        if (node && node->getStateSet())
        {
            osg::StateSet* ss = node->getStateSet();
            osg::Material* mat = dynamic_cast<osg::Material*>(ss->getAttribute(osg::StateAttribute::MATERIAL));
            if (mat)
            {
                float animationValue = 0.0f;
                
                switch (m_animationType)
                {
                    case AnimationType::Pulse:  // 脉冲动画：亮度周期性变化
                        animationValue = 0.5f * (std::sin(elapsed * 2.0f * osg::PI) + 1.0f);
                        break;
                        
                    case AnimationType::Blink:  // 闪烁动画：快速开关
                        animationValue = (static_cast<int>(elapsed * 4.0f) % 2 == 0) ? 1.0f : 0.2f;
                        break;
                        
                    case AnimationType::Gradient:  // 渐变动画：颜色渐变
                        animationValue = 0.5f * (std::sin(elapsed * 1.5f * osg::PI) + 1.0f);
                        break;
                        
                    default:
                        animationValue = 1.0f;
                        break;
                }
                
                osg::Vec4 baseEmission = mat->getEmission(osg::Material::FRONT_AND_BACK);
                
                // 根据动画类型调整发射光强度
                float intensity = 0.0f;
                if (m_animationType == AnimationType::Blink)
                {
                    intensity = animationValue;
                }
                else
                {
                    intensity = 0.3f + 0.7f * animationValue;
                }
                
                mat->setEmission(osg::Material::FRONT_AND_BACK, 
                               osg::Vec4(baseEmission.r() * intensity, 
                                       baseEmission.g() * intensity, 
                                       baseEmission.b() * intensity, 
                                       baseEmission.a()));
            }
        }
    }
}

void OsgContainer::mouseDoubleClickEvent(QMouseEvent *event)
{
    int button = 0;
    switch (event->button())
    {
    case Qt::LeftButton:
        button = 1;
        break;
    case Qt::MidButton:
        button = 2;
        break;
    case Qt::RightButton:
        button = 3;
        break;
    case Qt::NoButton:
        button = 0;
        break;
    default:
        button = 0;
        break;
    }
    setKeyboardModifiers(event);
    window->getEventQueue()->mouseDoubleButtonPress(event->x(), event->y(), button);

    QOpenGLWidget::mouseDoubleClickEvent(event);
}

void OsgContainer::mouseMoveEvent(QMouseEvent *event)
{
    setKeyboardModifiers(event);
    window->getEventQueue()->mouseMotion(event->x(), event->y());
    QOpenGLWidget::mouseMoveEvent(event);
}

void OsgContainer::wheelEvent(QWheelEvent *event)
{
    // 使用OSG默认缩放处理：转发滚轮事件到事件队列
    setKeyboardModifiers(event);
    window->getEventQueue()->mouseScroll(
        event->orientation() == Qt::Vertical ? (event->delta() < 0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN) : (event->delta() < 0 ? osgGA::GUIEventAdapter::SCROLL_LEFT : osgGA::GUIEventAdapter::SCROLL_RIGHT));
    QOpenGLWidget::wheelEvent(event);
}

// 拾取窗口坐标对应的世界坐标（最近命中点）
bool OsgContainer::pickAt(int x, int y, osg::Vec3d &world) const
{
    osg::Camera *cam = const_cast<OsgContainer *>(this)->getCamera();
    if (!cam || !root.valid())
        return false;
    osg::Viewport *vp = cam->getViewport();
    if (!vp)
        return false;
    // OSG窗口坐标原点在左下，Qt在左上，需翻转Y
    double wx = static_cast<double>(x);
    double wy = static_cast<double>(vp->height()) - 1.0 - static_cast<double>(y);

    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
        new osgUtil::LineSegmentIntersector(osgUtil::Intersector::WINDOW, wx, wy);
    osgUtil::IntersectionVisitor iv(intersector.get());
    cam->accept(iv);
    if (!intersector->containsIntersections())
        return false;
    const auto &hits = intersector->getIntersections();
    auto it = hits.begin();
    if (it == hits.end())
        return false;
    world = it->getWorldIntersectPoint();
    return true;
}

void OsgContainer::resizeEvent(QResizeEvent *event)
{
    const QSize &size = event->size();
    window->resized(x(), y(), size.width(), size.height());
    window->getEventQueue()->windowResize(x(), y(), size.width(), size.height());
    window->requestRedraw();
    QOpenGLWidget::resizeEvent(event);
}

void OsgContainer::moveEvent(QMoveEvent *event)
{
    const QPoint &pos = event->pos();
    window->resized(pos.x(), pos.y(), width(), height());
    window->getEventQueue()->windowResize(pos.x(), pos.y(), width(), height());
    QOpenGLWidget::moveEvent(event);
}
// 不再使用全局 timerEvent

void OsgContainer::paintGL()
{
    if (!isVisibleTo(QApplication::activeWindow()))
    {
        return;
    }

    // 检查上下文是否有效
    if (!context() || !context()->isValid())
    {
        qDebug() << "Invalid OpenGL context in paintGL";
        return;
    }

    // 性能优化：动画帧率限制
    if (m_highlightAnimationEnabled && m_animationType != AnimationType::None && !m_highlighted.empty())
    {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        qint64 minFrameInterval = 1000 / std::max(m_maxAnimationFPS, 1); // 防止除零
        
        if (currentTime - m_lastAnimationUpdate >= minFrameInterval)
        {
            updateAnimationEffects();
            m_lastAnimationUpdate = currentTime;
        }
    }

    frame();
}

void OsgContainer::init3D()
{
    root = new osg::Group;
    root->setName("root");

    setCamera(createCamera(0, 0, width(), height()));
    osg::ref_ptr<osgGA::MultiTouchTrackballManipulator> manipulator = new osgGA::MultiTouchTrackballManipulator();
    // 降低最小距离，避免近距离观察时被限制，允许贴近观察（OSG 3.6.5 无 setMinimumZoomScale）
    manipulator->setMinimumDistance(0.0);
    // 禁止甩动
    manipulator->setAllowThrow(false);
    // 固定垂直轴，避免中键平移产生轻微旋转/滚转
    manipulator->setVerticalAxisFixed(true);

    setCameraManipulator(manipulator);

    // 保留统计信息：添加 StatsHandler（按 F1 切换显示）；其余辅助 Handler 仍不启用以降低负载
    addEventHandler(new osgViewer::StatsHandler());
    addEventHandler(new osgGA::StateSetManipulator(this->getCamera()->getOrCreateStateSet()));
    // 让 OSG 根据硬件/驱动自动选择线程模型（在嵌入场景下通常仍可能为单线程）
    setThreadingModel(osgViewer::Viewer::AutomaticSelection);

    // 法线随缩放变换
    root->getOrCreateStateSet()->setMode(GL_RESCALE_NORMAL, osg::StateAttribute::ON);

    // 背面剔除
    root->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);

    // 设置无阴影的均匀光照
    setupUniformLighting();

    // 设置深度函数，确保正确的深度排序
    osg::ref_ptr<osg::Depth> depth = new osg::Depth;
    depth->setWriteMask(true);

    // 深度测试
    root->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    // 开启背面剔除，提升栅格化效率（若出现单面材质“消失”，可在材质层面处理双面）
    root->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);

    setSceneData(root);

    // 初始化持续渲染定时器
    if (!m_renderTimer)
    {
        m_renderTimer = new QTimer(this);
        m_renderTimer->setTimerType(Qt::CoarseTimer);
        m_renderTimer->setInterval(16); // 约60FPS
        connect(m_renderTimer, &QTimer::timeout, this, [this]
                { update(); });
        m_renderTimer->start(); // 立即开始持续渲染
    }
}

void OsgContainer::setupUniformLighting()
{
    // 创建无阴影的均匀光照系统
    osg::StateSet *stateSet = root->getOrCreateStateSet();

    // 启用双面光照
    osg::ref_ptr<osg::LightModel> lightModel = new osg::LightModel;
    lightModel->setTwoSided(true);
    lightModel->setAmbientIntensity(osg::Vec4(0.6f, 0.6f, 0.6f, 1.0f)); // 提高环境光强度
    stateSet->setAttributeAndModes(lightModel, osg::StateAttribute::ON);

    // 主光源 - 从前上方照射
    osg::ref_ptr<osg::Light> light0 = new osg::Light;
    light0->setLightNum(0);
    light0->setPosition(osg::Vec4(1.0f, 1.0f, 1.0f, 0.0f)); // 方向光
    light0->setDiffuse(osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f));
    light0->setSpecular(osg::Vec4(0.3f, 0.3f, 0.3f, 1.0f));
    light0->setAmbient(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

    osg::ref_ptr<osg::LightSource> lightSource0 = new osg::LightSource;
    lightSource0->setLight(light0);
    lightSource0->setLocalStateSetModes(osg::StateAttribute::ON);
    root->addChild(lightSource0);

    // 辅助光源1 - 从右侧照射，填补阴影
    osg::ref_ptr<osg::Light> light1 = new osg::Light;
    light1->setLightNum(1);
    light1->setPosition(osg::Vec4(1.0f, -0.5f, 0.0f, 0.0f)); // 方向光
    light1->setDiffuse(osg::Vec4(0.4f, 0.4f, 0.4f, 1.0f));
    light1->setSpecular(osg::Vec4(0.1f, 0.1f, 0.1f, 1.0f));
    light1->setAmbient(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

    osg::ref_ptr<osg::LightSource> lightSource1 = new osg::LightSource;
    lightSource1->setLight(light1);
    lightSource1->setLocalStateSetModes(osg::StateAttribute::ON);
    root->addChild(lightSource1);

    // 辅助光源2 - 从左侧照射，进一步减少阴影
    osg::ref_ptr<osg::Light> light2 = new osg::Light;
    light2->setLightNum(2);
    light2->setPosition(osg::Vec4(-1.0f, -0.5f, 0.0f, 0.0f)); // 方向光
    light2->setDiffuse(osg::Vec4(0.4f, 0.4f, 0.4f, 1.0f));
    light2->setSpecular(osg::Vec4(0.1f, 0.1f, 0.1f, 1.0f));
    light2->setAmbient(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

    osg::ref_ptr<osg::LightSource> lightSource2 = new osg::LightSource;
    lightSource2->setLight(light2);
    lightSource2->setLocalStateSetModes(osg::StateAttribute::ON);
    root->addChild(lightSource2);

    // 底部补光 - 从下方照射，消除底部阴影
    osg::ref_ptr<osg::Light> light3 = new osg::Light;
    light3->setLightNum(3);
    light3->setPosition(osg::Vec4(0.0f, 0.0f, -1.0f, 0.0f)); // 方向光
    light3->setDiffuse(osg::Vec4(0.3f, 0.3f, 0.3f, 1.0f));
    light3->setSpecular(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
    light3->setAmbient(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

    osg::ref_ptr<osg::LightSource> lightSource3 = new osg::LightSource;
    lightSource3->setLight(light3);
    lightSource3->setLocalStateSetModes(osg::StateAttribute::ON);
    root->addChild(lightSource3);

    // 启用光照
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHT0, osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHT1, osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHT2, osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHT3, osg::StateAttribute::ON);
}

osg::ref_ptr<osg::Camera> OsgContainer::createCamera(int x, int y, int w, int h)
{
    window = new osgViewer::GraphicsWindowEmbedded(x, y, w, h);
    osg::DisplaySettings *ds = osg::DisplaySettings::instance().get();
    // 与 QSurfaceFormat 对齐的多重采样设置
    ds->setNumMultiSamples(4);
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->windowDecoration = true;
    traits->x = x;
    traits->y = y;
    traits->width = w;
    traits->height = h;
    traits->doubleBuffer = true;
    traits->sharedContext = 0;
    traits->width = w;
    traits->height = h;
    traits->doubleBuffer = true;
    traits->sharedContext = 0;

    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setGraphicsContext(window);
    camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // 提高远裁剪面，减小近远比，避免缩放时被裁剪导致“消失”
    camera->setProjectionMatrixAsPerspective(30.0f, double(w) / double(h), 0.05f, 1e7f);
    camera->setNearFarRatio(0.00001); // 允许更小的近/远比例
    // 关闭小特征裁剪，减少远距离细节被剔除的概率
    camera->setCullingMode(camera->getCullingMode() & ~osg::CullSettings::SMALL_FEATURE_CULLING);
    // 使用包围体自动计算近远裁剪，避免平移后超出固定裁剪范围
    camera->setComputeNearFarMode(osg::CullSettings::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);
    camera->setClearColor(osg::Vec4(0.89, 0.90, 0.92, 1.0f));

    return camera.release();
}

// 简单高亮：为节点设置黄色自发光材质
void OsgContainer::highlightNode(osg::Node *node)
{
    if (!node)
        return;
        
    // 性能检查：节点数量限制
    if (m_maxHighlightedNodes > 0 && m_currentHighlightedCount >= m_maxHighlightedNodes)
    {
        qDebug() << "高亮节点数量已达到限制" << m_maxHighlightedNodes << "，跳过新节点";
        return;
    }
        
    // 若未保存原始状态集，则保存之
    if (m_savedStateSet.find(node) == m_savedStateSet.end())
    {
        m_savedStateSet[node] = node->getStateSet();
    }
    
    // 基于原始状态集克隆一个新的，并叠加高亮材质
    osg::ref_ptr<osg::StateSet> newSS = new osg::StateSet;
    if (m_savedStateSet[node].valid())
    {
        newSS->merge(*m_savedStateSet[node].get());
    }
    
    // 性能优化：根据性能模式决定是否应用复杂效果
    osg::ref_ptr<osg::StateSet> hl = createOptimizedHighlightStateSet(m_highlightMode);
    if (hl.valid())
    {
        newSS->merge(*hl.get());
    }
    
    node->setStateSet(newSS.get());
    m_highlighted.push_back(node);
    m_currentHighlightedCount++;
}

// 多节点高亮
void OsgContainer::highlightNodes(const std::vector<osg::Node *> &nodes)
{
    for (auto *n : nodes)
        highlightNode(n);
}

// 创建高亮状态集（支持多种高亮模式）
osg::ref_ptr<osg::StateSet> OsgContainer::createHighlightStateSet(HighlightMode mode)
{
    osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;
    
    // 根据高亮模式创建不同的材质效果
    osg::ref_ptr<osg::Material> mat = new osg::Material;
    
    // 定义各种颜色的发光效果
    osg::Vec4 ambient, diffuse, emission, specular;
    float shininess = 64.0f;
    
    switch (mode)
    {
        case HighlightMode::YellowGlow:  // 黄色发光（默认）
            ambient = osg::Vec4(1.0f, 0.8f, 0.2f, 1.0f);
            diffuse = osg::Vec4(1.0f, 0.9f, 0.4f, 1.0f);
            emission = osg::Vec4(0.8f, 0.6f, 0.0f, 1.0f);
            specular = osg::Vec4(1.0f, 1.0f, 0.6f, 1.0f);
            shininess = 64.0f;
            break;
            
        case HighlightMode::RedPulse:  // 红色脉冲
            ambient = osg::Vec4(1.0f, 0.2f, 0.2f, 1.0f);
            diffuse = osg::Vec4(1.0f, 0.4f, 0.4f, 1.0f);
            emission = osg::Vec4(0.8f, 0.1f, 0.1f, 1.0f);
            specular = osg::Vec4(1.0f, 0.6f, 0.6f, 1.0f);
            shininess = 32.0f;
            break;
            
        case HighlightMode::BlueGlow:  // 蓝色发光
            ambient = osg::Vec4(0.2f, 0.4f, 1.0f, 1.0f);
            diffuse = osg::Vec4(0.4f, 0.6f, 1.0f, 1.0f);
            emission = osg::Vec4(0.1f, 0.3f, 0.8f, 1.0f);
            specular = osg::Vec4(0.6f, 0.8f, 1.0f, 1.0f);
            shininess = 96.0f;
            break;
            
        case HighlightMode::GreenGlow:  // 绿色发光
            ambient = osg::Vec4(0.2f, 1.0f, 0.2f, 1.0f);
            diffuse = osg::Vec4(0.4f, 1.0f, 0.4f, 1.0f);
            emission = osg::Vec4(0.1f, 0.8f, 0.1f, 1.0f);
            specular = osg::Vec4(0.6f, 1.0f, 0.6f, 1.0f);
            shininess = 80.0f;
            break;
            
        case HighlightMode::OrangeGlow:  // 橙色发光
            ambient = osg::Vec4(1.0f, 0.5f, 0.0f, 1.0f);
            diffuse = osg::Vec4(1.0f, 0.7f, 0.2f, 1.0f);
            emission = osg::Vec4(0.8f, 0.4f, 0.0f, 1.0f);
            specular = osg::Vec4(1.0f, 0.8f, 0.4f, 1.0f);
            shininess = 72.0f;
            break;
            
        case HighlightMode::CyanGlow:  // 青色发光
            ambient = osg::Vec4(0.0f, 1.0f, 1.0f, 1.0f);
            diffuse = osg::Vec4(0.2f, 1.0f, 1.0f, 1.0f);
            emission = osg::Vec4(0.0f, 0.8f, 0.8f, 1.0f);
            specular = osg::Vec4(0.4f, 1.0f, 1.0f, 1.0f);
            shininess = 88.0f;
            break;
            
        case HighlightMode::PurpleGlow:  // 紫色发光
            ambient = osg::Vec4(0.8f, 0.2f, 0.8f, 1.0f);
            diffuse = osg::Vec4(0.9f, 0.4f, 0.9f, 1.0f);
            emission = osg::Vec4(0.6f, 0.1f, 0.6f, 1.0f);
            specular = osg::Vec4(0.9f, 0.6f, 0.9f, 1.0f);
            shininess = 56.0f;
            break;
            
        case HighlightMode::WhiteGlow:  // 白色发光
            ambient = osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            diffuse = osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            emission = osg::Vec4(0.7f, 0.7f, 0.7f, 1.0f);
            specular = osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            shininess = 128.0f;
            break;
            
        case HighlightMode::Wireframe:  // 线框模式
            ambient = osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f);
            diffuse = osg::Vec4(1.0f, 1.0f, 0.2f, 1.0f);
            emission = osg::Vec4(0.8f, 0.8f, 0.0f, 1.0f);
            specular = osg::Vec4(1.0f, 1.0f, 0.4f, 1.0f);
            shininess = 16.0f;
            break;
            
        default:  // 默认黄色发光
            ambient = osg::Vec4(1.0f, 0.8f, 0.2f, 1.0f);
            diffuse = osg::Vec4(1.0f, 0.9f, 0.4f, 1.0f);
            emission = osg::Vec4(0.8f, 0.6f, 0.0f, 1.0f);
            specular = osg::Vec4(1.0f, 1.0f, 0.6f, 1.0f);
            shininess = 64.0f;
            break;
    }
    
    // 应用强度调整
    ambient *= m_highlightIntensity;
    diffuse *= m_highlightIntensity;
    emission *= m_highlightIntensity;
    specular *= m_highlightIntensity;
    
    // 设置材质属性
    mat->setAmbient(osg::Material::FRONT_AND_BACK, ambient);
    mat->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse);
    mat->setEmission(osg::Material::FRONT_AND_BACK, emission);
    mat->setSpecular(osg::Material::FRONT_AND_BACK, specular);
    mat->setShininess(osg::Material::FRONT_AND_BACK, shininess);
    
    ss->setAttributeAndModes(mat.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    
    // 禁用深度测试，使高亮始终可见
    ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    
    // 禁用面剔除，显示所有面的高亮
    ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    
    // 增强线宽（如果节点有线框模式）
    osg::ref_ptr<osg::LineWidth> lw = new osg::LineWidth(3.0f);
    ss->setAttributeAndModes(lw.get(), osg::StateAttribute::ON);
    
    // 添加顶点颜色乘数（如果原模型有顶点颜色）
    ss->setMode(GL_COLOR_MATERIAL, osg::StateAttribute::ON);
    
    // 应用动画效果
    if (m_highlightAnimationEnabled && m_animationType != AnimationType::None)
    {
        applyAnimationEffects(ss, mode);
    }
    
    // 线框模式特殊设置
    if (mode == HighlightMode::Wireframe)
    {
        ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        ss->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE), 
                                osg::StateAttribute::ON);
    }
    
    return ss.release();
}

// 性能优化的高亮状态集创建
osg::ref_ptr<osg::StateSet> OsgContainer::createOptimizedHighlightStateSet(HighlightMode mode)
{
    // 根据性能模式选择不同的实现
    switch (m_performanceMode)
    {
        case PerformanceMode::HighQuality:
            // 高质量模式：使用完整效果
            return createHighlightStateSet(mode);
            
        case PerformanceMode::HighPerformance:
            // 高性能模式：使用最简化的效果
            return createSimpleHighlightStateSet(mode);
            
        case PerformanceMode::Balanced:
        default:
            // 平衡模式：在性能和效果之间平衡
            return createBalancedHighlightStateSet(mode);
    }
}

// 简化高亮状态集（高性能模式）
osg::ref_ptr<osg::StateSet> OsgContainer::createSimpleHighlightStateSet(HighlightMode mode)
{
    osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;
    
    // 简化材质：只设置基本的自发光
    osg::ref_ptr<osg::Material> mat = new osg::Material;
    
    // 简化颜色设置
    osg::Vec4 emissionColor;
    switch (mode)
    {
        case HighlightMode::YellowGlow:
            emissionColor = osg::Vec4(1.0f, 0.8f, 0.2f, 1.0f);
            break;
        case HighlightMode::RedPulse:
            emissionColor = osg::Vec4(1.0f, 0.3f, 0.3f, 1.0f);
            break;
        case HighlightMode::BlueGlow:
            emissionColor = osg::Vec4(0.3f, 0.6f, 1.0f, 1.0f);
            break;
        case HighlightMode::GreenGlow:
            emissionColor = osg::Vec4(0.3f, 1.0f, 0.3f, 1.0f);
            break;
        case HighlightMode::OrangeGlow:
            emissionColor = osg::Vec4(1.0f, 0.6f, 0.2f, 1.0f);
            break;
        case HighlightMode::CyanGlow:
            emissionColor = osg::Vec4(0.2f, 1.0f, 1.0f, 1.0f);
            break;
        case HighlightMode::PurpleGlow:
            emissionColor = osg::Vec4(0.8f, 0.3f, 0.8f, 1.0f);
            break;
        case HighlightMode::WhiteGlow:
            emissionColor = osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            break;
        case HighlightMode::Wireframe:
            emissionColor = osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f);
            break;
        default:
            emissionColor = osg::Vec4(1.0f, 0.8f, 0.2f, 1.0f);
            break;
    }
    
    // 应用强度调整
    emissionColor *= m_highlightIntensity;
    mat->setEmission(osg::Material::FRONT_AND_BACK, emissionColor);
    
    ss->setAttributeAndModes(mat.get(), osg::StateAttribute::ON);
    
    // 最小化渲染状态设置
    ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    
    // 线框模式特殊处理
    if (mode == HighlightMode::Wireframe)
    {
        ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        ss->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE), 
                                osg::StateAttribute::ON);
    }
    
    return ss.release();
}

// 平衡高亮状态集（平衡模式）
osg::ref_ptr<osg::StateSet> OsgContainer::createBalancedHighlightStateSet(HighlightMode mode)
{
    osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;
    
    // 简化材质设置：只设置关键属性
    osg::ref_ptr<osg::Material> mat = new osg::Material;
    
    osg::Vec4 ambient, diffuse, emission;
    switch (mode)
    {
        case HighlightMode::YellowGlow:
            ambient = osg::Vec4(0.8f, 0.6f, 0.2f, 1.0f);
            diffuse = osg::Vec4(1.0f, 0.9f, 0.4f, 1.0f);
            emission = osg::Vec4(0.6f, 0.4f, 0.0f, 1.0f);
            break;
        case HighlightMode::RedPulse:
            ambient = osg::Vec4(0.8f, 0.2f, 0.2f, 1.0f);
            diffuse = osg::Vec4(1.0f, 0.4f, 0.4f, 1.0f);
            emission = osg::Vec4(0.6f, 0.1f, 0.1f, 1.0f);
            break;
        case HighlightMode::BlueGlow:
            ambient = osg::Vec4(0.2f, 0.4f, 0.8f, 1.0f);
            diffuse = osg::Vec4(0.4f, 0.6f, 1.0f, 1.0f);
            emission = osg::Vec4(0.1f, 0.3f, 0.6f, 1.0f);
            break;
        case HighlightMode::GreenGlow:
            ambient = osg::Vec4(0.2f, 0.8f, 0.2f, 1.0f);
            diffuse = osg::Vec4(0.4f, 1.0f, 0.4f, 1.0f);
            emission = osg::Vec4(0.1f, 0.6f, 0.1f, 1.0f);
            break;
        case HighlightMode::OrangeGlow:
            ambient = osg::Vec4(0.8f, 0.4f, 0.0f, 1.0f);
            diffuse = osg::Vec4(1.0f, 0.7f, 0.2f, 1.0f);
            emission = osg::Vec4(0.6f, 0.3f, 0.0f, 1.0f);
            break;
        case HighlightMode::CyanGlow:
            ambient = osg::Vec4(0.0f, 0.8f, 0.8f, 1.0f);
            diffuse = osg::Vec4(0.2f, 1.0f, 1.0f, 1.0f);
            emission = osg::Vec4(0.0f, 0.6f, 0.6f, 1.0f);
            break;
        case HighlightMode::PurpleGlow:
            ambient = osg::Vec4(0.6f, 0.2f, 0.6f, 1.0f);
            diffuse = osg::Vec4(0.9f, 0.4f, 0.9f, 1.0f);
            emission = osg::Vec4(0.4f, 0.1f, 0.4f, 1.0f);
            break;
        case HighlightMode::WhiteGlow:
            ambient = osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f);
            diffuse = osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            emission = osg::Vec4(0.5f, 0.5f, 0.5f, 1.0f);
            break;
        case HighlightMode::Wireframe:
            ambient = osg::Vec4(0.8f, 0.8f, 0.0f, 1.0f);
            diffuse = osg::Vec4(1.0f, 1.0f, 0.2f, 1.0f);
            emission = osg::Vec4(0.6f, 0.6f, 0.0f, 1.0f);
            break;
        default:
            ambient = osg::Vec4(0.8f, 0.6f, 0.2f, 1.0f);
            diffuse = osg::Vec4(1.0f, 0.9f, 0.4f, 1.0f);
            emission = osg::Vec4(0.6f, 0.4f, 0.0f, 1.0f);
            break;
    }
    
    // 应用强度调整
    ambient *= m_highlightIntensity;
    diffuse *= m_highlightIntensity;
    emission *= m_highlightIntensity;
    
    // 设置关键材质属性
    mat->setAmbient(osg::Material::FRONT_AND_BACK, ambient);
    mat->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse);
    mat->setEmission(osg::Material::FRONT_AND_BACK, emission);
    mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0.3f, 0.3f, 0.3f, 1.0f) * m_highlightIntensity);
    mat->setShininess(osg::Material::FRONT_AND_BACK, 32.0f);
    
    ss->setAttributeAndModes(mat.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    
    // 禁用深度测试和面剔除
    ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    
    // 简化线宽设置
    osg::ref_ptr<osg::LineWidth> lw = new osg::LineWidth(2.0f);
    ss->setAttributeAndModes(lw.get(), osg::StateAttribute::ON);
    
    // 启用顶点颜色
    ss->setMode(GL_COLOR_MATERIAL, osg::StateAttribute::ON);
    
    // 线框模式特殊设置
    if (mode == HighlightMode::Wireframe)
    {
        ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        ss->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE), 
                                osg::StateAttribute::ON);
    }
    
    return ss.release();
}

// 应用动画效果到状态集
void OsgContainer::applyAnimationEffects(osg::StateSet* ss, HighlightMode mode)
{
    if (!ss || m_animationType == AnimationType::None)
        return;
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (m_animationStartTime == 0)
        m_animationStartTime = currentTime;
    
    float elapsed = (currentTime - m_animationStartTime) / 1000.0f * m_animationSpeed;
    float animationValue = 0.0f;
    
    switch (m_animationType)
    {
        case AnimationType::Pulse:  // 脉冲动画：亮度周期性变化
            animationValue = 0.5f * (std::sin(elapsed * 2.0f * osg::PI) + 1.0f);
            break;
            
        case AnimationType::Blink:  // 闪烁动画：快速开关
            animationValue = (static_cast<int>(elapsed * 4.0f) % 2 == 0) ? 1.0f : 0.2f;
            break;
            
        case AnimationType::Gradient:  // 渐变动画：颜色渐变
            animationValue = 0.5f * (std::sin(elapsed * 1.5f * osg::PI) + 1.0f);
            break;
            
        default:
            animationValue = 1.0f;
            break;
    }
    
    // 获取材质并应用动画效果
    osg::Material* mat = dynamic_cast<osg::Material*>(ss->getAttribute(osg::StateAttribute::MATERIAL));
    if (mat)
    {
        osg::Vec4 baseAmbient, baseDiffuse, baseEmission, baseSpecular;
        baseAmbient = mat->getAmbient(osg::Material::FRONT_AND_BACK);
        baseDiffuse = mat->getDiffuse(osg::Material::FRONT_AND_BACK);
        baseEmission = mat->getEmission(osg::Material::FRONT_AND_BACK);
        baseSpecular = mat->getSpecular(osg::Material::FRONT_AND_BACK);
        
        // 根据动画类型调整颜色
        if (m_animationType == AnimationType::Gradient && mode == HighlightMode::RedPulse)
        {
            // 红脉冲特殊处理：颜色强度变化
            float intensity = 0.3f + 0.7f * animationValue;
            mat->setEmission(osg::Material::FRONT_AND_BACK, 
                           osg::Vec4(baseEmission.r() * intensity, 
                                   baseEmission.g() * intensity, 
                                   baseEmission.b() * intensity, 
                                   baseEmission.a()));
        }
        else if (m_animationType == AnimationType::Blink)
        {
            // 闪烁：完全开关效果
            float intensity = animationValue;
            mat->setEmission(osg::Material::FRONT_AND_BACK, 
                           osg::Vec4(baseEmission.r() * intensity, 
                                   baseEmission.g() * intensity, 
                                   baseEmission.b() * intensity, 
                                   baseEmission.a()));
        }
        else
        {
            // 脉冲和渐变：亮度调节
            float intensity = 0.5f + 0.5f * animationValue;
            mat->setEmission(osg::Material::FRONT_AND_BACK, 
                           osg::Vec4(baseEmission.r() * intensity, 
                                   baseEmission.g() * intensity, 
                                   baseEmission.b() * intensity, 
                                   baseEmission.a()));
        }
    }
}

// 高亮模式控制
void OsgContainer::setHighlightMode(HighlightMode mode)
{
    m_highlightMode = mode;
    
    // 如果当前有高亮的节点，重新应用高亮效果
    if (!m_highlighted.empty())
    {
        // 保存当前高亮的节点
        std::vector<osg::Node*> currentNodes;
        for (const auto& nodePtr : m_highlighted)
        {
            if (nodePtr.valid())
                currentNodes.push_back(nodePtr.get());
        }
        
        // 清除现有高亮
        clearHighlight();
        
        // 重新应用新模式的高亮
        for (osg::Node* node : currentNodes)
        {
            highlightNode(node);
        }
    }
}

// 清空所有高亮并恢复原始状态集
void OsgContainer::clearHighlight()
{
    for (auto &kv : m_savedStateSet)
    {
        osg::Node *node = kv.first.get();
        if (node)
        {
            node->setStateSet(kv.second.get());
        }
    }
    m_savedStateSet.clear();
    m_highlighted.clear();
    m_currentHighlightedCount = 0;  // 重置计数
}

// 选中若干节点后视图适配（透视相机）：按包围球与FOV计算距离，保持当前视线方向不变
void OsgContainer::fitToNodes(const std::vector<osg::Node *> &nodes)
{
    if (nodes.empty())
        return;
    osg::BoundingBox bb;
    for (auto *n : nodes)
    {
        if (!n)
            continue;
        osg::ComputeBoundsVisitor cbv;
        const_cast<osg::Node *>(n)->accept(cbv);
        osg::BoundingBox nb = cbv.getBoundingBox();
        if (nb.valid())
            bb.expandBy(nb);
    }
    if (!bb.valid())
        return;

    osg::Camera *cam = getCamera();
    if (!cam)
        return;

    osg::Vec3d center = bb.center();
    double radius = (bb._max - bb._min).length() * 0.5;
    if (radius <= 1e-6)
        radius = 1.0;

    double fovy, aspect, zNear, zFar;
    if (!cam->getProjectionMatrixAsPerspective(fovy, aspect, zNear, zFar))
    {
        fovy = 30.0;
        aspect = 1.0;
        zNear = 0.05;
        zFar = 1e7;
    }
    double fovyRad = osg::DegreesToRadians(fovy);
    double dist = radius / std::sin(0.5 * fovyRad);
    if (!std::isfinite(dist) || dist < 1e-3)
        dist = 10.0;

    osg::Vec3d eye, look, up;
    cam->getViewMatrixAsLookAt(eye, look, up);
    osg::Vec3d viewDir = (look - eye);
    if (viewDir.length2() < 1e-12)
        viewDir = osg::Vec3d(0, -1, 0);
    viewDir.normalize();

    osg::Vec3d newCenter = center;
    osg::Vec3d newEye = newCenter - viewDir * dist;

    if (auto *mt = dynamic_cast<osgGA::MultiTouchTrackballManipulator *>(getCameraManipulator()))
    {
        mt->setCenter(newCenter);
        mt->setDistance(dist);
        // 同步home，确保后续交互围绕目标
        mt->setHomePosition(newEye, newCenter, up);
        mt->home(0.0);
    }
    else if (auto *tb = dynamic_cast<osgGA::TrackballManipulator *>(getCameraManipulator()))
    {
        tb->setCenter(newCenter);
        tb->setDistance(dist);
        tb->setHomePosition(newEye, newCenter, up);
        tb->home(0.0);
    }

    // 自动计算近远裁剪，避免对焦后向右平移时被裁剪
    cam->setComputeNearFarMode(osg::CullSettings::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);
}

// 构建包围盒线框
osg::ref_ptr<osg::Geode> OsgContainer::buildBBoxGeode(const osg::BoundingBox &bb) const
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array();
    verts->reserve(8);
    verts->push_back(osg::Vec3(bb.xMin(), bb.yMin(), bb.zMin())); // 0
    verts->push_back(osg::Vec3(bb.xMax(), bb.yMin(), bb.zMin())); // 1
    verts->push_back(osg::Vec3(bb.xMax(), bb.yMax(), bb.zMin())); // 2
    verts->push_back(osg::Vec3(bb.xMin(), bb.yMax(), bb.zMin())); // 3
    verts->push_back(osg::Vec3(bb.xMin(), bb.yMin(), bb.zMax())); // 4
    verts->push_back(osg::Vec3(bb.xMax(), bb.yMin(), bb.zMax())); // 5
    verts->push_back(osg::Vec3(bb.xMax(), bb.yMax(), bb.zMax())); // 6
    verts->push_back(osg::Vec3(bb.xMin(), bb.yMax(), bb.zMax())); // 7
    geom->setVertexArray(verts.get());

    osg::ref_ptr<osg::DrawElementsUInt> lines = new osg::DrawElementsUInt(GL_LINES);
    auto addEdge = [&](unsigned a, unsigned b)
    { lines->push_back(a); lines->push_back(b); };
    // 下四条
    addEdge(0, 1);
    addEdge(1, 2);
    addEdge(2, 3);
    addEdge(3, 0);
    // 上四条
    addEdge(4, 5);
    addEdge(5, 6);
    addEdge(6, 7);
    addEdge(7, 4);
    // 立边四条
    addEdge(0, 4);
    addEdge(1, 5);
    addEdge(2, 6);
    addEdge(3, 7);
    geom->addPrimitiveSet(lines.get());

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    colors->push_back(osg::Vec4(1.0f, 0.95f, 0.2f, 1.0f));
    geom->setColorArray(colors.get(), osg::Array::BIND_OVERALL);

    geode->addDrawable(geom.get());
    osg::StateSet *ss = geode->getOrCreateStateSet();
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    osg::ref_ptr<osg::LineWidth> lw = new osg::LineWidth(2.0f);
    ss->setAttributeAndModes(lw.get(), osg::StateAttribute::ON);
    return geode.release();
}

// 显示/隐藏场景包围盒
void OsgContainer::setShowSceneBBox(bool on)
{
    m_showBBox = on;
    if (!root.valid())
        return;
    if (m_bboxGeode.valid())
    {
        root->removeChild(m_bboxGeode.get());
        m_bboxGeode = nullptr;
    }
    if (!on)
        return;

    osg::ComputeBoundsVisitor cbv;
    root->accept(cbv);
    osg::BoundingBox bb = cbv.getBoundingBox();
    if (!bb.valid())
        return;
    m_bboxGeode = buildBBoxGeode(bb);
    root->addChild(m_bboxGeode.get());
}

// 充满视图：以整棵场景为对象
void OsgContainer::fitToView()
{
    if (!root.valid() || root->getNumChildren() == 0)
        return;
    osg::ComputeBoundsVisitor cbv;
    root->accept(cbv);
    osg::BoundingBox bb = cbv.getBoundingBox();
    if (!bb.valid())
        return;
    std::vector<osg::Node *> nodes;
    nodes.reserve(root->getNumChildren());
    // 直接用包围盒计算（与 fitToNodes 一致逻辑）
    osg::Camera *cam = getCamera();
    if (!cam)
        return;
    osg::Vec3d center = bb.center();
    double radius = (bb._max - bb._min).length() * 0.5;
    if (radius <= 1e-6)
        radius = 1.0;

    double fovy, aspect, zNear, zFar;
    if (!cam->getProjectionMatrixAsPerspective(fovy, aspect, zNear, zFar))
    {
        fovy = 30.0;
        aspect = 1.0;
        zNear = 0.05;
        zFar = 1e7;
    }
    // 同时考虑水平与垂直 FOV，避免左右贴边
    double halfFovY = osg::DegreesToRadians(fovy) * 0.5;
    double halfFovX = std::atan(std::tan(halfFovY) * aspect);
    double distY = radius / std::sin(halfFovY);
    double distX = radius / std::sin(halfFovX);
    double dist = std::max(distX, distY);
    if (!std::isfinite(dist) || dist < 1e-3)
        dist = 10.0;

    osg::Vec3d eye, look, up;
    cam->getViewMatrixAsLookAt(eye, look, up);
    osg::Vec3d viewDir = (look - eye);
    if (viewDir.length2() < 1e-12)
        viewDir = osg::Vec3d(0, -1, 0);
    viewDir.normalize();

    osg::Vec3d newCenter = center;
    osg::Vec3d newEye = newCenter - viewDir * dist;

    if (auto *mt = dynamic_cast<osgGA::MultiTouchTrackballManipulator *>(getCameraManipulator()))
    {
        mt->setCenter(newCenter);
        mt->setDistance(dist);
        mt->setHomePosition(newEye, newCenter, up);
        mt->home(0.0);
    }
    else if (auto *tb = dynamic_cast<osgGA::TrackballManipulator *>(getCameraManipulator()))
    {
        tb->setCenter(newCenter);
        tb->setDistance(dist);
        tb->setHomePosition(newEye, newCenter, up);
        tb->home(0.0);
    }
}
