#pragma once

#include <QOpenGLWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <osgViewer/Viewer>
#include <osgViewer/GraphicsWindow>
#include <osgGA/TrackballManipulator>
#include <osgDB/ReadFile>
#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/Group>
#include <osgText/Text>
#include <osg/Node>
#include <QLabel>
#include <osgUtil/LineSegmentIntersector>
#include <osg/PolygonMode>
#include <osg/StateSet>
#include <osg/Geometry>
#include <osg/Geode>

class OSGWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit OSGWidget(QWidget* parent = nullptr);
    ~OSGWidget() override;

    bool loadModel(const QString& path);
    osg::Node* currentNode() const;

signals:
    void statsUpdated(double fps, double memMB);
    void nodePicked(osg::Node* node);
    void propertiesUpdated(const QString& text);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> _gw;
    std::unique_ptr<osgViewer::Viewer> _viewer;
    QTimer _timer;
    QElapsedTimer _fpsTimer;
    int _frameCount = 0;
    double _lastFps = 0.0;
    osg::ref_ptr<osg::Group> _root;
    osg::ref_ptr<osg::Group> _sceneRoot;
    osg::ref_ptr<osg::Camera> _hudCamera;
    osg::ref_ptr<osgText::Text> _hudText;
    osg::observer_ptr<osg::Node> _selected;
    osg::ref_ptr<osg::StateAttribute> _savedMaterial;

    void createScene();
    void createHud();
    osgGA::EventQueue* eventQueue() const;
    void pickAt(int x, int y);
    void applyHighlight(osg::Node* node);
    void clearHighlight();
    QString buildProperties(osg::Node* node) const;
};
