#include "WindowQt.h"

#include <QApplication>
#include <QDebug>
#include <QOpenGLContext>
#include <QResizeEvent>
#include <QSurfaceFormat>

QSurfaceFormat defaultFormat() {
    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CoreProfile);
#ifndef NDEBUG
    format.setOption(QSurfaceFormat::DebugContext);
#endif
    return format;
}

WindowQt::WindowQt() : WindowQt(defaultFormat()) {}

WindowQt::WindowQt(const QSurfaceFormat& format)
    : glcontext(new QOpenGLContext), updatePending(false), initialized(false) {
    QSurfaceFormat f(format);
    f.setRenderableType(QSurfaceFormat::OpenGL);

    setSurfaceType(OpenGLSurface);
    create();

    glcontext->setFormat(format);
    if (!glcontext->create()) {
        qDebug() << "Could not create OpenGL context.";
        QApplication::quit();
    }
}

WindowQt::~WindowQt() {
    if (initialized) {
        makeCurrent();

        deinitializeGL();

        doneCurrent();
    }
}

QOpenGLContext* WindowQt::context() { return glcontext.data(); }

void WindowQt::makeCurrent() { glcontext->makeCurrent(this); }

void WindowQt::doneCurrent() { glcontext->doneCurrent(); }

void WindowQt::resizeEvent(QResizeEvent* event) {
    resize(event);
    paint();
}

void WindowQt::exposeEvent(QExposeEvent* /*unused*/) { paint(); }

void WindowQt::initialize() {
    makeCurrent();

    initializeGL();

    doneCurrent();

    initialized = true;
}

void WindowQt::resize(QResizeEvent* event) {
    if (!initialized) {
        initialize();
    }

    makeCurrent();

    QResizeEvent deviceSpecificResizeEvent(event->size() * devicePixelRatio(), event->oldSize() * devicePixelRatio());

    resizeGL(&deviceSpecificResizeEvent);

    doneCurrent();
}

void WindowQt::paint() {
    if (!initialized) {
        initialize();
    }

    if (!isExposed()) {
        return;
    }

    updatePending = false;

    makeCurrent();

    paintGL();

    glcontext->swapBuffers(this);

    doneCurrent();
}

void WindowQt::updateGL() {
    if (!updatePending) {
        updatePending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool WindowQt::event(QEvent* event) {
    switch (event->type()) {
        case QEvent::UpdateRequest:
            paint();
            return true;

        case QEvent::Enter:
            enterEvent(event);
            return true;

        case QEvent::Leave:
            leaveEvent(event);
            return true;

        default:
            return QWindow::event(event);
    }
}

void WindowQt::paintGL() { updateGL(); }

void WindowQt::mousePressEvent(QMouseEvent* /*event*/) {}

void WindowQt::mouseMoveEvent(QMouseEvent* /*event*/) {}

void WindowQt::mouseReleaseEvent(QMouseEvent* /*event*/) {}

void WindowQt::wheelEvent(QWheelEvent* /*event*/) {}

void WindowQt::initializeGL() {}

void WindowQt::deinitializeGL() {}

void WindowQt::resizeGL(QResizeEvent* /*unused*/) {}

void WindowQt::enterEvent(QEvent* /*unused*/) {}

void WindowQt::leaveEvent(QEvent* /*unused*/) {}

void WindowQt::backgroundColorRChanged(QString /*unused*/) {}

void WindowQt::backgroundColorGChanged(QString /*unused*/) {}

void WindowQt::backgroundColorBChanged(QString /*unused*/) {}

void WindowQt::fontColorRChanged(QString /*unused*/) {}

void WindowQt::fontColorGChanged(QString /*unused*/) {}

void WindowQt::fontColorBChanged(QString /*unused*/) {}

void WindowQt::resetTransform3D() {}

void WindowQt::superSamplingChanged(int /*unused*/) {}

void WindowQt::toggleDistanceField(bool /*unused*/) {}
