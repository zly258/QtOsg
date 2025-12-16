#include "OSGWidget.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QSize>
#include <QString>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

OSGWidget::OSGWidget(QWidget* parent) : QOpenGLWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
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
    _viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    _viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    _root = new osg::Group;
    _sceneRoot = new osg::Group;
    _root->addChild(_sceneRoot.get());
    createScene();
    createHud();
    _viewer->setSceneData(_root.get());
}

void OSGWidget::createScene() {
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(new osg::ShapeDrawable(new osg::Box(osg::Vec3(0, 0, 0), 1.0f)));
    _sceneRoot->addChild(geode.get());
}

void OSGWidget::createHud() {
    _hudCamera = new osg::Camera;
    _hudCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    _hudCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, width(), 0, height()));
    _hudCamera->setViewMatrix(osg::Matrix::identity());
    _hudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    _hudCamera->setRenderOrder(osg::Camera::POST_RENDER);
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    _hudText = new osgText::Text;
    _hudText->setCharacterSize(16.0f);
    _hudText->setPosition(osg::Vec3(10.0f, height() - 22.0f, 0.0f));
    geode->addDrawable(_hudText.get());
    _hudCamera->addChild(geode.get());
    _root->addChild(_hudCamera.get());
}

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
    if (_hudCamera.valid()) {
        _hudCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, w, 0, h));
        if (_hudText.valid()) _hudText->setPosition(osg::Vec3(10.0f, h - 22.0f, 0.0f));
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
        if (_hudText.valid()) {
            QString s = QString("FPS: %1  MEM: %2 MB").arg(_lastFps, 0, 'f', 1).arg(memMB, 0, 'f', 1);
            _hudText->setText(s.toStdString());
        }
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
        if (event->angleDelta().y() > 0) q->mouseScroll(osgGA::GUIEventAdapter::SCROLL_UP);
        else q->mouseScroll(osgGA::GUIEventAdapter::SCROLL_DOWN);
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
    return true;
}
