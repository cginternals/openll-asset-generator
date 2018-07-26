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

glbinding::ProcAddress WindowQt::getProcAddress(const char * name)
{
    if (name == nullptr)
    {
        return nullptr;
    }

    const auto symbol = std::string(name);

    #if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
    const auto qtSymbol = QByteArray::fromStdString(symbol);
    #else
    const auto qtSymbol = QByteArray::fromRawData(symbol.c_str(), symbol.size());
    #endif
    return glcontext->getProcAddress(qtSymbol);
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

void WindowQt::dtAlgorithmChanged(int /*unused*/) {}

void WindowQt::packingAlgoChanged(int /*unused*/) {}

void WindowQt::packingSizeChanged(int /*unused*/) {}

void WindowQt::dtThresholdChanged(QString /*unused*/) {}

void WindowQt::fontNameChanged(QString /*unused*/) {}

void WindowQt::fontSizeChanged(QString /*unused*/) {}

void WindowQt::drBlackChanged(QString /*unused*/) {}

void WindowQt::drWhiteChanged(QString /*unused*/) {}

void WindowQt::paddingChanged(QString /*unused*/) {}

void WindowQt::resetTransform3D() {}

void WindowQt::triggerNewDT(){}

void WindowQt::superSamplingChanged(int /*unused*/) {}

void WindowQt::toggleDistanceField(bool /*unused*/) {}

void WindowQt::exportGlyphAtlas(){}
