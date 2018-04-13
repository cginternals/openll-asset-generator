#pragma once

#include <QScopedPointer>
#include <QWindow>

class QSurfaceFormat;
class QOpenGLContext;

/* This class is taken from cginternals/globjects examples: qtexample, texture
 * and edited accordingly. It redirects GL functionality to its subclass(es), so that
 * GL functions which are included via Qt don't interfere with GL functions included via glbinding.
 */
class WindowQt : public QWindow {
    Q_OBJECT

   public:
    WindowQt();
    explicit WindowQt(const QSurfaceFormat& format);
    virtual ~WindowQt();

    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void exposeEvent(QExposeEvent* event) override;
    bool event(QEvent* event) override;

    virtual void enterEvent(QEvent* event);
    virtual void leaveEvent(QEvent* event);

    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void wheelEvent(QWheelEvent* event) override;

    /*
     * Make OpenGL Context current context.
     */
    void makeCurrent();
    /*
     * Call this when OpenGL Context was made current,
     * after finishing the algorithm that needed that current context.
     */
    void doneCurrent();

    QOpenGLContext* context();

    void updateGL();

   public slots:
    virtual void backgroundColorRChanged(QString value);
    virtual void backgroundColorGChanged(QString value);
    virtual void backgroundColorBChanged(QString value);
    virtual void fontColorRChanged(QString value);
    virtual void fontColorGChanged(QString value);
    virtual void fontColorBChanged(QString value);
    virtual void dtAlgorithmChanged(int index);
    virtual void packingAlgoChanged(int index);
    virtual void packingSizeChanged(int index);
    virtual void dtThresholdChanged(QString value);
    virtual void fontNameChanged(QString value);
    virtual void fontSizeChanged(QString value);
    virtual void drBlackChanged(QString value);
    virtual void drWhiteChanged(QString value);
    virtual void resetTransform3D();
    virtual void triggerNewDT();
    virtual void superSamplingChanged(int index);
    virtual void toggleDistanceField(bool activated);
    virtual void exportGlyphAtlas();

   protected:
    QScopedPointer<QOpenGLContext> glcontext;

    bool updatePending;
    bool initialized;

    void initialize();
    void resize(QResizeEvent* event);
    void paint();

    virtual void initializeGL();
    virtual void deinitializeGL();
    virtual void resizeGL(QResizeEvent* event);
    virtual void paintGL();
};
