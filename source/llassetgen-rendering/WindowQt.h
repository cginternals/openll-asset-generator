#pragma once

#pragma warning(push)
#pragma warning(disable: 4127)
#include <QScopedPointer>
#include <QWindow>
#pragma warning(pop)

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
    bool event(QEvent* event);

    virtual void enterEvent(QEvent* event);
    virtual void leaveEvent(QEvent* event);

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

   protected:
    QScopedPointer<QOpenGLContext> m_context;

    bool m_updatePending;
    bool m_initialized;

    void initialize();
    void resize(QResizeEvent* event);
    void paint();

    virtual void initializeGL();
    virtual void resizeGL(QResizeEvent* event);
    virtual void paintGL();
};