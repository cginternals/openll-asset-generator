#include <algorithm>
#include <iostream>
#include <string>

#include <ft2build.h>  // NOLINT include order required by freetype
#include FT_FREETYPE_H

#include <QApplication>
#include <QBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QResizeEvent>
#include <QValidator>

#include <glm/glm.hpp>

#include <glbinding/ContextInfo.h>
#include <glbinding/Version.h>
#include <glbinding/gl/gl.h>

#include <globjects/base/File.h>
#include <globjects/globjects.h>
#include <globjects/logging.h>

#include <globjects/Buffer.h>
#include <globjects/Program.h>
#include <globjects/Shader.h>
#include <globjects/Texture.h>
#include <globjects/VertexArray.h>
#include <globjects/VertexAttributeBinding.h>

#include <llassetgen/llassetgen.h>

#include "WindowQt.h"

using namespace gl;

/* Taken from cginternals/globjects,
 * example: qtexample, texture
 * edited
 */

/*
 * This class is used to display GL rendered content in a Qt Window.
 * It overrides all functions from WindowQt which use GL functionality.
 * It handles the GL context and loads the shader from file.
 * Some functions are connected to the GUI using Qt signals and slots.
 */
class Window : public WindowQt {
   public:
    explicit Window(const QSurfaceFormat &format) : WindowQt(format) {}

    virtual ~Window() {}

    virtual void initializeGL() override {
        globjects::init();

        std::cout << std::endl
                  << "OpenGL Version:  " << glbinding::ContextInfo::version() << std::endl
                  << "OpenGL Vendor:   " << glbinding::ContextInfo::vendor() << std::endl
                  << "OpenGL Renderer: " << glbinding::ContextInfo::renderer() << std::endl
                  << std::endl;

        globjects::DebugMessage::enable();

#ifdef __APPLE__
        globjects::Shader::clearGlobalReplacements();
        globjects::Shader::globalReplace("#version 140", "#version 150");

        globjects::debug() << "Using global OS X shader replacement '#version 140' -> '#version 150'" << std::endl;
#endif

        // get glyph atlas

        // TODO load using own png loader instead of Qt (blocked: wait for this feature to be merged into master, then
        // pull). TODO: Make sure this file structure is the same on other OS
        auto path = QApplication::applicationDirPath();
        auto *image = new QImage(path + "/../../data/llassetgen-rendering/testfontatlas_rgb.png");

        // mirrored: Qt flips images after
        // loading; meant as convenience, but
        // we need it to flip back here.
        auto imageFormatted = image->convertToFormat(QImage::Format_RGBA8888).mirrored(false, true);
        auto imageData = imageFormatted.bits();

        texture = globjects::Texture::createDefault(GL_TEXTURE_2D);
        texture->image2D(0, GL_RGBA8, image->width(), image->height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, imageData);
        // TODO: Willy told me that green and blue channels are swapped, that's why GL_BGRA is used here; we also might
        // ignore this, since we use black&white image data here?

        cornerBuffer = globjects::Buffer::create();
        program = globjects::Program::create();
        vao = globjects::VertexArray::create();

        // openll-asset-generator/data/llassetgen-rendering
        const std::string dataPath = path.toStdString() + "/../../data/llassetgen-rendering";

        vertexShaderSource = globjects::Shader::sourceFromFile(dataPath + "/shader.vert");
        vertexShaderTemplate = globjects::Shader::applyGlobalReplacements(vertexShaderSource.get());
        vertexShader = globjects::Shader::create(GL_VERTEX_SHADER, vertexShaderTemplate.get());

        fragmentShaderSource = globjects::Shader::sourceFromFile(dataPath + "/shader.frag");
        fragmentShaderTemplate = globjects::Shader::applyGlobalReplacements(fragmentShaderSource.get());
        fragmentShader = globjects::Shader::create(GL_FRAGMENT_SHADER, fragmentShaderTemplate.get());

        program->attach(vertexShader.get(), fragmentShader.get());

        cornerBuffer->setData(
            std::array<glm::vec2, 4>{{glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(0, 1), glm::vec2(1, 1)}},
            GL_STATIC_DRAW);

        vao->binding(0)->setAttribute(0);
        vao->binding(0)->setBuffer(cornerBuffer.get(), 0, sizeof(glm::vec2));
        vao->binding(0)->setFormat(2, GL_FLOAT);
        vao->enable(0);

        program->setUniform("glyphs", samplerIndex);
        program->setUniform("fontColor", fontColor);
    }

    virtual void deinitializeGL() override {
        texture.reset(nullptr);

        cornerBuffer.reset(nullptr);
        program.reset(nullptr);
        vertexShaderSource.reset(nullptr);
        vertexShaderTemplate.reset(nullptr);
        vertexShader.reset(nullptr);
        fragmentShaderSource.reset(nullptr);
        fragmentShaderTemplate.reset(nullptr);
        fragmentShader.reset(nullptr);
        vao.reset(nullptr);
    }

    virtual void resizeGL(QResizeEvent *event) override {
        glViewport(0, 0, event->size().width(), event->size().height());
    }

    virtual void paintGL() override {
        glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (texture) {
            texture->bindActive(samplerIndex);
        }

        program->use();
        program->setUniform("fontColor", fontColor);
        vao->drawArrays(GL_TRIANGLE_STRIP, 0, 4);
        program->release();

        if (texture) {
            texture->unbindActive(samplerIndex);
        }

        glDisable(GL_BLEND);
    }

    virtual void keyPressEvent(QKeyEvent *event) override {
        makeCurrent();

        switch (event->key()) {
            case Qt::Key_F5:
                vertexShaderSource->reload();
                fragmentShaderSource->reload();
                updateGL();
                break;
            case Qt::Key_Escape:
                qApp->quit();
                break;
            default:
                break;
        }
        doneCurrent();
    }

   public slots:
    virtual void backgroundColorRChanged(QString value) override {
        int red = value.toInt();
        backgroundColor.r = red / 255.f;
        paint();
    }

    virtual void backgroundColorGChanged(QString value) override {
        int green = value.toInt();
        backgroundColor.g = green / 255.f;
        paint();
    }

    virtual void backgroundColorBChanged(QString value) override {
        int blue = value.toInt();
        backgroundColor.b = blue / 255.f;
        paint();
    }

    virtual void fontColorRChanged(QString value) override {
        int red = value.toInt();
        fontColor.r = red / 255.f;
        paint();
    }

    virtual void fontColorGChanged(QString value) override {
        int green = value.toInt();
        fontColor.g = green / 255.f;
        paint();
    }

    virtual void fontColorBChanged(QString value) override {
        int blue = value.toInt();
        fontColor.b = blue / 255.f;
        paint();
    }

   protected:
    std::unique_ptr<globjects::Buffer> cornerBuffer;
    std::unique_ptr<globjects::Program> program;
    std::unique_ptr<globjects::File> vertexShaderSource;
    std::unique_ptr<globjects::AbstractStringSource> vertexShaderTemplate;
    std::unique_ptr<globjects::Shader> vertexShader;
    std::unique_ptr<globjects::File> fragmentShaderSource;
    std::unique_ptr<globjects::AbstractStringSource> fragmentShaderTemplate;
    std::unique_ptr<globjects::Shader> fragmentShader;
    std::unique_ptr<globjects::VertexArray> vao;

    std::unique_ptr<globjects::Texture> texture = nullptr;

    glm::vec4 backgroundColor = glm::vec4(1.f, 1.f, 1.f, 1.f);
    glm::vec4 fontColor = glm::vec4(0.f, 0.f, 0.f, 1.f);
    int samplerIndex = 0;
};

/* This function creates GUI elements and connects them to their correspondent functions using Qt signal-slot.
 * A QApplication offers a window, that must not be destroyed to make QApplication work properly.
 */
void setupGUI(QMainWindow *window) {
    // from globjects
    QSurfaceFormat format;
#ifdef __APPLE__
    format.setVersion(4, 1);  // ToDo: which version is supported on macOS?
    format.setProfile(QSurfaceFormat::CoreProfile);
#else
    format.setVersion(4, 3);
#endif
    format.setDepthBufferSize(16);

    Window *glwindow = new Window(format);

    window->setMinimumSize(640, 480);
    window->setWindowTitle("Open Font Asset Generator");

    auto colorValidator = new QIntValidator(0, 255);

    // FONT COLOR
    auto fontColorGroupBox = new QGroupBox("Font Color");
    fontColorGroupBox->setMaximumHeight(150);
    auto fontColorLayout = new QFormLayout();

    fontColorGroupBox->setLayout(fontColorLayout);

    // font Color RED
    auto *fontR = new QLineEdit();
    auto *labelFontR = new QLabel("R:");
    labelFontR->setMaximumWidth(90);
    fontR->setValidator(colorValidator);
    fontR->setPlaceholderText("0");
    fontR->setMaximumWidth(45);

    QObject::connect(fontR, SIGNAL(textEdited(QString)), glwindow, SLOT(fontColorRChanged(QString)));
    fontColorLayout->addRow(labelFontR, fontR);

    // font Color GREEN
    auto *fontG = new QLineEdit();
    auto *labelFontG = new QLabel("G:");
    labelFontG->setMaximumWidth(90);
    fontG->setValidator(colorValidator);
    fontG->setPlaceholderText("0");
    fontG->setMaximumWidth(45);

    QObject::connect(fontG, SIGNAL(textEdited(QString)), glwindow, SLOT(fontColorGChanged(QString)));
    fontColorLayout->addRow(labelFontG, fontG);

    // font Color BLUE
    auto *fontB = new QLineEdit();
    auto *labelFontB = new QLabel("B:");
    labelFontB->setMaximumWidth(90);
    fontB->setValidator(colorValidator);
    fontB->setPlaceholderText("0");
    fontB->setMaximumWidth(45);

    QObject::connect(fontB, SIGNAL(textEdited(QString)), glwindow, SLOT(fontColorBChanged(QString)));
    fontColorLayout->addRow(labelFontB, fontB);

    // BACKGROUND COLOR

    auto backgroundColorGroupBox = new QGroupBox("Background Color");
    backgroundColorGroupBox->setMaximumHeight(150);
    auto backgroundColorLayout = new QFormLayout();
    backgroundColorGroupBox->setLayout(backgroundColorLayout);

    // Background Color RED
    auto *backgroundR = new QLineEdit();
    auto *labelR = new QLabel("R");
    labelR->setMaximumWidth(90);
    backgroundR->setValidator(colorValidator);
    backgroundR->setPlaceholderText("255");
    backgroundR->setMaximumWidth(45);

    QObject::connect(backgroundR, SIGNAL(textEdited(QString)), glwindow, SLOT(backgroundColorRChanged(QString)));

    backgroundColorLayout->addRow(labelR, backgroundR);

    // Background Color GREEN
    auto *backgroundG = new QLineEdit();
    auto *labelG = new QLabel("G");
    labelG->setMaximumWidth(90);
    backgroundG->setValidator(colorValidator);
    backgroundG->setPlaceholderText("255");
    backgroundG->setMaximumWidth(45);

    QObject::connect(backgroundG, SIGNAL(textEdited(QString)), glwindow, SLOT(backgroundColorGChanged(QString)));

    backgroundColorLayout->addRow(labelG, backgroundG);

    // Background Color BLUE
    auto *backgroundB = new QLineEdit();
    auto *labelB = new QLabel("B");
    labelB->setMaximumWidth(90);
    backgroundB->setValidator(colorValidator);
    backgroundB->setPlaceholderText("255");
    backgroundB->setMaximumWidth(45);

    QObject::connect(backgroundB, SIGNAL(textEdited(QString)), glwindow, SLOT(backgroundColorBChanged(QString)));

    backgroundColorLayout->addRow(labelB, backgroundB);

    // gather all parameters into one layout (separately from the gl window)
    auto *guiLayout = new QBoxLayout(QBoxLayout::LeftToRight);
    guiLayout->addWidget(backgroundColorGroupBox, 0, Qt::AlignLeft);
    guiLayout->addWidget(fontColorGroupBox, 0, Qt::AlignLeft);

    auto *mainLayout = new QBoxLayout(QBoxLayout::TopToBottom);
    mainLayout->addLayout(guiLayout, 0);
    mainLayout->addWidget(QWidget::createWindowContainer(glwindow));

    // since window already has a special layout, we have to put our layout on a widget
    // and then set it as central widget
    auto central = new QWidget();
    central->setLayout(mainLayout);

    window->setCentralWidget(central);
}

int main(int argc, char **argv) {
    llassetgen::init();

    std::unique_ptr<llassetgen::DistanceTransform> dt(new llassetgen::DeadReckoning());

    QApplication app(argc, argv);

    auto path = app.applicationDirPath();
    dt->importPng(path.toStdString() + "/../../data/llassetgen-rendering/testfontatlas.png");
    dt->transform();
    dt->exportPng(path.toStdString() + "/../../data/llassetgen-rendering/testfontatlasDT.png", -20, 50, 8);
    // TODO: don't export, but use as texture directly
    // TODO: exported png is corrupted, wait for update/fix

    QMainWindow *window = new QMainWindow();
    setupGUI(window);
    window->show();

    return app.exec();
}
