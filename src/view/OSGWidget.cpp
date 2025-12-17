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
#include <osg/ComputeBoundsVisitor>
#include <osg/CullFace>
#include <osgViewer/ViewerEventHandlers>
#include <cmath>

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
    osg::DisplaySettings::instance()->setNumMultiSamples(4);
    cam->getOrCreateStateSet()->setMode(GL_MULTISAMPLE, osg::StateAttribute::ON);
    _viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    _manip = new osgGA::TrackballManipulator;
    _manip->setAllowThrow(false);
    _manip->setMinimumDistance(0.0);
    _viewer->setCameraManipulator(_manip.get());
    _root = new osg::Group;
    _sceneRoot = new osg::Group;
    _root->addChild(_sceneRoot.get());
    _viewer->setSceneData(_root.get());
    updateProjection();
    _viewer->addEventHandler(new osgViewer::StatsHandler);
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
        updateProjection();
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
    _pressPos = event->pos();
    _dragging = false;
    if (event->button() == Qt::MiddleButton) {
        _panning = true;
        _panStart = event->pos();
        return;
    }
    if (auto* q = eventQueue()) {
        unsigned int button = 0;
        if (event->button() == Qt::LeftButton) button = osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON;
        else if (event->button() == Qt::MiddleButton) button = osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON;
        else if (event->button() == Qt::RightButton) button = osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON;
        q->mouseButtonPress(event->pos().x(), event->pos().y(), button);
    }
}

void OSGWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        _panning = false;
        return;
    }
    if (auto* q = eventQueue()) {
        unsigned int button = 0;
        if (event->button() == Qt::LeftButton) button = osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON;
        else if (event->button() == Qt::MiddleButton) button = osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON;
        else if (event->button() == Qt::RightButton) button = osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON;
        q->mouseButtonRelease(event->pos().x(), event->pos().y(), button);
    }
    if (event->button() == Qt::LeftButton && !_dragging) {
        pickAt(event->pos().x(), event->pos().y());
    }
}

void OSGWidget::mouseMoveEvent(QMouseEvent* event) {
    if ((event->buttons() & Qt::LeftButton) == Qt::LeftButton) {
        if ((event->pos() - _pressPos).manhattanLength() > 3) _dragging = true;
    }
    if (_panning && _manip.valid()) {
        QPoint delta = event->pos() - _panStart;
        _panStart = event->pos();
        osg::Vec3d eye, center, up;
        _manip->getTransformation(eye, center, up);
        osg::Vec3d forward = center - eye; forward.normalize();
        osg::Vec3d right = up ^ forward; right.normalize();
        osg::Vec3d trueUp = forward ^ right; trueUp.normalize();
        int w = width(); int h = std::max(1, height());
        double aspect = static_cast<double>(w) / static_cast<double>(h);
        double perPixelX = 0.0, perPixelY = 0.0;
        if (_ortho) {
            osg::Node* n = currentNode();
            double r = 1.0; if (n) { osg::BoundingSphere bs = n->getBound(); if (bs.radius()>0.0) r = bs.radius(); }
            double s = r * _orthoScale;
            perPixelX = (2.0 * s * aspect) / static_cast<double>(w);
            perPixelY = (2.0 * s) / static_cast<double>(h);
        } else {
            double fovy=30.0, aOut=aspect, zN=1.0, zF=10000.0;
            _viewer->getCamera()->getProjectionMatrixAsPerspective(fovy, aOut, zN, zF);
            double dist = (center - eye).length();
            double perPixelYBase = 2.0 * dist * std::tan(osg::DegreesToRadians(fovy*0.5)) / static_cast<double>(h);
            perPixelY = perPixelYBase; perPixelX = perPixelYBase * aspect;
        }
        double kx = _panSpeed * perPixelX;
        double ky = _panSpeed * perPixelY;
        osg::Vec3d trans = right * (delta.x()*kx) + trueUp * (delta.y()*ky);
        eye += trans; center += trans;
        _manip->setTransformation(eye, center, up);
        return;
    }
    if (auto* q = eventQueue()) {
        q->mouseMotion(event->pos().x(), event->pos().y());
    }
}

void OSGWidget::wheelEvent(QWheelEvent* event) {
    if (_ortho) {
        if (event->angleDelta().y() > 0) {
            _orthoScale /= 1.1;
            if (_orthoScale < 1e-6) _orthoScale = 1e-6;
        } else {
            _orthoScale *= 1.1;
            if (_orthoScale > 1e6) _orthoScale = 1e6;
        }
        updateProjection();
    } else {
        if (auto* q = eventQueue()) {
            if (event->angleDelta().y() > 0) q->mouseScroll(osgGA::GUIEventAdapter::SCROLL_UP);
            else q->mouseScroll(osgGA::GUIEventAdapter::SCROLL_DOWN);
        }
    }
}

void OSGWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_W) { toggleWireframe(); return; }
    if (event->key() == Qt::Key_B) { toggleBackface(); return; }
    if (event->key() == Qt::Key_L) { toggleLighting(); return; }
    if (event->key() == Qt::Key_S) {
        if (auto* q = eventQueue()) q->keyPress('s');
        return;
    }
    if (auto* q = eventQueue()) { q->keyPress(event->key()); }
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
    updateProjection();
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
        lines << QString("位置: (%1, %2, %3)").arg(t.x(), 0, 'f', 3).arg(t.y(), 0, 'f', 3).arg(t.z(), 0, 'f', 3);
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
                lines << QString("材质漫反射: (%1, %2, %3, %4)").arg(d.r(),0,'f',2).arg(d.g(),0,'f',2).arg(d.b(),0,'f',2).arg(d.a(),0,'f',2);
            }
            int texCount=0; for (int unit=0; unit<8; ++unit) { if (ss->getTextureAttribute(unit, osg::StateAttribute::TEXTURE)) ++texCount; }
            lines << QString("纹理: %1 单元").arg(texCount);
        }
    }
    return lines.join("\n");
}

void OSGWidget::updateProjection() {
    if (!_viewer) return;
    osg::Camera* cam = _viewer->getCamera();
    int w = width();
    int h = std::max(1, height());
    double aspect = static_cast<double>(w) / static_cast<double>(h);
    if (_ortho) {
        double r = 1.0;
        osg::Node* n = currentNode();
        if (n) {
            osg::BoundingSphere bs = n->getBound();
            if (bs.radius() > 0.0) r = bs.radius();
        }
        double s = r * _orthoScale;
        double top = s;
        double bottom = -s;
        double right = s * aspect;
        double left = -right;
        cam->setProjectionMatrixAsOrtho(left, right, bottom, top, 1.0, 10000.0);
    } else {
        cam->setProjectionMatrixAsPerspective(30.0, aspect, 1.0, 10000.0);
    }
}

void OSGWidget::applyRenderStates() {
    osg::StateSet* ss = _root->getOrCreateStateSet();
    if (_wireframe) {
        osg::ref_ptr<osg::PolygonMode> pm = new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
        ss->setAttributeAndModes(pm.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    } else {
        ss->removeAttribute(osg::StateAttribute::POLYGONMODE);
    }
    if (_backface) {
        osg::ref_ptr<osg::CullFace> cf = new osg::CullFace(osg::CullFace::BACK);
        ss->setAttributeAndModes(cf.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    } else {
        ss->removeAttribute(osg::StateAttribute::CULLFACE);
    }
    ss->setMode(GL_LIGHTING, _lighting ? osg::StateAttribute::ON : osg::StateAttribute::OFF);
}

void OSGWidget::toggleWireframe() {
    _wireframe = !_wireframe;
    applyRenderStates();
}

void OSGWidget::toggleBackface() {
    _backface = !_backface;
    applyRenderStates();
}

void OSGWidget::toggleLighting() {
    _lighting = !_lighting;
    applyRenderStates();
}

void OSGWidget::setOrthographic(bool enable) {
    _ortho = enable;
    updateProjection();
}

void OSGWidget::setStandardView(ViewDir dir) {
    osg::Vec3d center(0.0, 0.0, 0.0);
    double r = 1.0;
    osg::Node* n = currentNode();
    if (n) {
        osg::BoundingSphere bs = n->getBound();
        center = bs.center();
        if (bs.radius() > 0.0) r = bs.radius();
    }
    double dist = std::max(r * 2.0, 1.0);
    osg::Vec3d eye;
    osg::Vec3d up;
    switch (dir) {
        case Front: eye = center + osg::Vec3d(0.0, 0.0, dist); up = osg::Vec3d(0.0, 1.0, 0.0); break;
        case Back: eye = center + osg::Vec3d(0.0, 0.0, -dist); up = osg::Vec3d(0.0, 1.0, 0.0); break;
        case Left: eye = center + osg::Vec3d(-dist, 0.0, 0.0); up = osg::Vec3d(0.0, 1.0, 0.0); break;
        case Right: eye = center + osg::Vec3d(dist, 0.0, 0.0); up = osg::Vec3d(0.0, 1.0, 0.0); break;
        case Top: eye = center + osg::Vec3d(0.0, dist, 0.0); up = osg::Vec3d(0.0, 0.0, -1.0); break;
        case Bottom: eye = center + osg::Vec3d(0.0, -dist, 0.0); up = osg::Vec3d(0.0, 0.0, 1.0); break;
    }
    if (_manip.valid()) {
        _manip->setHomePosition(eye, center, up);
        _manip->home(0.0);
    }
}

void OSGWidget::clearSceneGraph() {
    if (_sceneRoot.valid()) {
        _sceneRoot->removeChildren(0, _sceneRoot->getNumChildren());
    }
    clearHighlight();
    emit propertiesUpdated("");
}
