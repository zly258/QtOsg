#include "OSGWidget.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QSize>
#include <QString>
#include <QSurfaceFormat>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif
#include <osg/DisplaySettings>
#include <osgUtil/IntersectionVisitor>
#include <osg/MatrixTransform>
#include <osg/Material>

OSGWidget::OSGWidget(QWidget* parent) : QOpenGLWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    QSurfaceFormat fmt = format();
    fmt.setSamples(4);
    setFormat(fmt);
    connect(&_timer, &QTimer::timeout, this, QOverload<>::of(&OSGWidget::update));
    _timer.start(16);
    _fpsTimer.start();
}

OSGWidget::~OSGWidget() {
    makeCurrent();
    _viewer.reset();
    doneCurrent();
}

void OSGWidget::initializeGL() {
    _gw = new osgViewer::GraphicsWindowEmbedded(0, 0, width(), height());
    _viewer = std::make_unique<osgViewer::Viewer>();
    osg::Camera* cam = _viewer->getCamera();
    cam->setViewport(0, 0, width(), height());
    cam->setGraphicsContext(_gw.get());
    double aspect = static_cast<double>(width()) / static_cast<double>(std::max(1, height()));
    cam->setProjectionMatrixAsPerspective(30.0, aspect, 1.0, 10000.0);
    osg::DisplaySettings::instance()->setNumMultiSamples(4);
    cam->getOrCreateStateSet()->setMode(GL_MULTISAMPLE, osg::StateAttribute::ON);
    _viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    _viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    _root = new osg::Group;
    _sceneRoot = new osg::Group;
    _root->addChild(_sceneRoot.get());
    _viewer->setSceneData(_root.get());
}

void OSGWidget::createScene() {}

void OSGWidget::createHud() {}

void OSGWidget::resizeGL(int w, int h) {
    if (_gw.valid()) {
        _gw->resized(0, 0, w, h);
    }
    if (_viewer) {
        osg::Camera* cam = _viewer->getCamera();
        cam->setViewport(0, 0, w, h);
        double aspect = static_cast<double>(w) / static_cast<double>(std::max(1, h));
        cam->setProjectionMatrixAsPerspective(30.0, aspect, 1.0, 10000.0);
    }
    
}

void OSGWidget::paintGL() {
    if (_viewer) {
        _viewer->frame();
        _frameCount++;
        if (_fpsTimer.elapsed() >= 1000) {
            _lastFps = static_cast<double>(_frameCount) * 1000.0 / static_cast<double>(_fpsTimer.elapsed());
            _frameCount = 0;
            _fpsTimer.restart();
        }
#ifdef _WIN32
        SIZE_T ws = 0;
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
            ws = pmc.WorkingSetSize;
        }
        double memMB = static_cast<double>(ws) / (1024.0 * 1024.0);
#else
        double memMB = 0.0;
#endif
        emit statsUpdated(_lastFps, memMB);
    }
}

osgGA::EventQueue* OSGWidget::eventQueue() const {
    if (_gw.valid()) return _gw->getEventQueue();
    return nullptr;
}

void OSGWidget::mousePressEvent(QMouseEvent* event) {
    if (auto* q = eventQueue()) {
        unsigned int button = 0;
        if (event->button() == Qt::LeftButton) button = osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON;
        else if (event->button() == Qt::MiddleButton) button = osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON;
        else if (event->button() == Qt::RightButton) button = osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON;
        q->mouseButtonPress(event->pos().x(), event->pos().y(), button);
    }
    if (event->button() == Qt::LeftButton) {
        pickAt(event->pos().x(), event->pos().y());
    }
}

void OSGWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (auto* q = eventQueue()) {
        unsigned int button = 0;
        if (event->button() == Qt::LeftButton) button = osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON;
        else if (event->button() == Qt::MiddleButton) button = osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON;
        else if (event->button() == Qt::RightButton) button = osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON;
        q->mouseButtonRelease(event->pos().x(), event->pos().y(), button);
    }
}

void OSGWidget::mouseMoveEvent(QMouseEvent* event) {
    if (auto* q = eventQueue()) {
        q->mouseMotion(event->pos().x(), event->pos().y());
    }
}

void OSGWidget::wheelEvent(QWheelEvent* event) {
    if (auto* q = eventQueue()) {
        if (event->angleDelta().y() > 0) q->mouseScroll(osgGA::GUIEventAdapter::SCROLL_DOWN);
        else q->mouseScroll(osgGA::GUIEventAdapter::SCROLL_UP);
    }
}

void OSGWidget::keyPressEvent(QKeyEvent* event) {
    if (auto* q = eventQueue()) {
        q->keyPress(event->key());
    }
}

void OSGWidget::keyReleaseEvent(QKeyEvent* event) {
    if (auto* q = eventQueue()) {
        q->keyRelease(event->key());
    }
}

void OSGWidget::resizeEvent(QResizeEvent* event) {
    QOpenGLWidget::resizeEvent(event);
    if (auto* q = eventQueue()) {
        const QSize s = event->size();
        q->windowResize(0, 0, s.width(), s.height());
    }
}

bool OSGWidget::loadModel(const QString& path) {
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(path.toStdString());
    if (!node) return false;
    _sceneRoot->removeChildren(0, _sceneRoot->getNumChildren());
    _sceneRoot->addChild(node.get());
    clearHighlight();
    return true;
}

osg::Node* OSGWidget::currentNode() const {
    if (!_sceneRoot.valid()) return nullptr;
    if (_sceneRoot->getNumChildren() == 0) return nullptr;
    return _sceneRoot->getChild(0);
}
void OSGWidget::pickAt(int x, int y) {
    if (!_viewer) return;
    osg::ref_ptr<osgUtil::LineSegmentIntersector> picker =
        new osgUtil::LineSegmentIntersector(osgUtil::Intersector::WINDOW, x, height() - y);
    osgUtil::IntersectionVisitor iv(picker.get());
    _viewer->getCamera()->accept(iv);
    if (picker->containsIntersections()) {
        const auto& isect = picker->getIntersections().begin()->nodePath;
        osg::Node* hitNode = nullptr;
        for (auto it = isect.rbegin(); it != isect.rend(); ++it) {
            if ((*it)->asGeode()) { hitNode = *it; break; }
        }
        if (!hitNode && !isect.empty()) hitNode = isect.back();
        if (hitNode) {
            applyHighlight(hitNode);
            emit nodePicked(hitNode);
            emit propertiesUpdated(buildProperties(hitNode));
        }
    } else {
        clearHighlight();
        emit nodePicked(nullptr);
        emit propertiesUpdated("");
    }
}

void OSGWidget::applyHighlight(osg::Node* node) {
    if (!node) return;
    clearHighlight();
    _selected = node;
    osg::Geode* geode = node->asGeode();
    if (geode) {
        osg::StateSet* ss = geode->getOrCreateStateSet();
        _savedMaterial = ss->getAttribute(osg::StateAttribute::MATERIAL);
        
        osg::ref_ptr<osg::Material> mat = new osg::Material;
        mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
        ss->setAttributeAndModes(mat.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    }
}

void OSGWidget::clearHighlight() {
    if (!_selected.valid()) return;
    osg::Geode* geode = _selected->asGeode();
    if (geode) {
        osg::StateSet* ss = geode->getStateSet();
        if (ss) {
            ss->removeAttribute(osg::StateAttribute::MATERIAL);
            if (_savedMaterial.valid()) {
                ss->setAttributeAndModes(_savedMaterial.get(), osg::StateAttribute::ON);
            }
        }
    }
    _selected = nullptr;
    _savedMaterial = nullptr;
}

QString OSGWidget::buildProperties(osg::Node* node) const {
    if (!node) return QString();
    QStringList lines;
    lines << QString("名称: %1").arg(QString::fromStdString(node->getName()));
    lines << QString("类型: %1").arg(QString::fromLatin1(node->className()));
    const osg::MatrixTransform* mt = dynamic_cast<const osg::MatrixTransform*>(node);
    if (!mt && node->getNumParents()>0) {
        mt = dynamic_cast<const osg::MatrixTransform*>(node->getParent(0));
    }
    if (mt) {
        osg::Vec3 t = mt->getMatrix().getTrans();
        lines << QString("位置: (%.3f, %.3f, %.3f)").arg(t.x(), 0, 'f', 3).arg(t.y(), 0, 'f', 3).arg(t.z(), 0, 'f', 3);
    }
    const osg::Geode* geode = node->asGeode();
    if (geode) {
        size_t vertCount = 0; size_t primCount = 0;
        for (unsigned int i=0; i<geode->getNumDrawables(); ++i) {
            const osg::Geometry* g = dynamic_cast<const osg::Geometry*>(geode->getDrawable(i));
            if (!g) continue;
            const osg::Array* va = g->getVertexArray();
            if (va) vertCount += va->getNumElements();
            primCount += g->getNumPrimitiveSets();
        }
        lines << QString("几何信息: %1 顶点, %2 图元组").arg((qulonglong)vertCount).arg((qulonglong)primCount);
        const osg::StateSet* ss = geode->getStateSet();
        if (ss) {
            const osg::Material* m = dynamic_cast<const osg::Material*>(ss->getAttribute(osg::StateAttribute::MATERIAL));
            if (m) {
                osg::Vec4 d = m->getDiffuse(osg::Material::FRONT);
                lines << QString("材质漫反射: (%.2f, %.2f, %.2f, %.2f)").arg(d.r(),0,'f',2).arg(d.g(),0,'f',2).arg(d.b(),0,'f',2).arg(d.a(),0,'f',2);
            }
            int texCount=0; for (int unit=0; unit<8; ++unit) { if (ss->getTextureAttribute(unit, osg::StateAttribute::TEXTURE)) ++texCount; }
            lines << QString("纹理: %1 单元").arg(texCount);
        }
    }
    return lines.join("\n");
}
