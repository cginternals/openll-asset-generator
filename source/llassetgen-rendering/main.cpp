#include <iostream>
#include <algorithm>
#include <string>

#include <ft2build.h>  // NOLINT include order required by freetype
#include FT_FREETYPE_H

#pragma warning(push)
#pragma warning(disable: 4127)
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QMainWindow>
#include <QResizeEvent>
#include <QSurfaceFormat>
#pragma warning(pop)


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <glbinding/gl/gl.h>
#include <glbinding/ContextInfo.h>
#include <glbinding/Version.h>

#include <globjects/globjects.h>
#include <globjects/base/File.h>
#include <globjects/logging.h>

#include <globjects/Buffer.h>
#include <globjects/Program.h>
#include <globjects/Shader.h>
#include <globjects/VertexArray.h>
#include <globjects/VertexAttributeBinding.h>
#include <globjects/base/StaticStringSource.h>


#include <llassetgen/llassetgen.h>

//#include "GLWidget.h"

#include "WindowQt.h"

using namespace gl;
/*Taken from cginternals/globjects, edited*/

class Window : public WindowQt
{
public:
    Window(QSurfaceFormat & format)
        : WindowQt(format)
    {
    }

    virtual ~Window()
    {
    }

    virtual void initializeGL() override
    {
        globjects::init();

        std::cout << std::endl
            << "OpenGL Version:  " << glbinding::ContextInfo::version() << std::endl
            << "OpenGL Vendor:   " << glbinding::ContextInfo::vendor() << std::endl
            << "OpenGL Renderer: " << glbinding::ContextInfo::renderer() << std::endl << std::endl;

        globjects::DebugMessage::enable();

#ifdef __APPLE__
        Shader::clearGlobalReplacements();
        Shader::globalReplace("#version 140", "#version 150");

        debug() << "Using global OS X shader replacement '#version 140' -> '#version 150'" << std::endl;
#endif

        m_cornerBuffer = new globjects::Buffer();
        m_program = new globjects::Program();
        m_vao = new globjects::VertexArray();

        //openll-asset-generator/data/llassetgen-rendering"; 
        const std::string dataPath = "./data/llassetgen-rendering";
        m_program->attach(
            globjects::Shader::fromFile(GL_VERTEX_SHADER, dataPath + "/shader.vert"),
            globjects::Shader::fromFile(GL_FRAGMENT_SHADER, dataPath + "/shader.frag"));

        m_cornerBuffer->setData(std::array<glm::vec2, 4>{ {
                glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(0, 1), glm::vec2(1, 1) } }, GL_STATIC_DRAW);

        m_vao->binding(0)->setAttribute(0);
        m_vao->binding(0)->setBuffer(m_cornerBuffer, 0, sizeof(glm::vec2));
        m_vao->binding(0)->setFormat(2, GL_FLOAT);
        m_vao->enable(0);
    }

    virtual void resizeGL(QResizeEvent * event) override
    {
        glViewport(0, 0, event->size().width(), event->size().height());
    }

    virtual void paintGL() override
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_program->use();
        m_vao->drawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }


    virtual void keyPressEvent(QKeyEvent * event) override
    {
        makeCurrent();

        switch (event->key())
        {
        case Qt::Key_F5:
            globjects::File::reloadAll();
            break;
        default:
            break;
        }
        doneCurrent();
    }


protected:
    globjects::ref_ptr<globjects::Buffer> m_cornerBuffer;
    globjects::ref_ptr<globjects::Program> m_program;
    globjects::ref_ptr<globjects::VertexArray> m_vao;
};



int main(int argc, char** argv) {
    llassetgen::init();

    QApplication app(argc, argv);


    // from globjects
    QSurfaceFormat format;
#ifdef __APPLE__
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
#else
    format.setVersion(3, 1);
#endif
    format.setDepthBufferSize(16);

    Window * glwindow = new Window(format);

    QMainWindow window;
    window.setMinimumSize(640, 480);
    window.setWindowTitle("Open Font Asset Generator");
    window.setCentralWidget(QWidget::createWindowContainer(glwindow));

    window.show();
    
    return app.exec();
}
