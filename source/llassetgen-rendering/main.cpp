#include <algorithm>
#include <iostream>
#include <string>

#include <ft2build.h>  // NOLINT include order required by freetype
#include FT_FREETYPE_H

#include <QApplication>
#include <QBoxLayout>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QResizeEvent>
#include <QValidator>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

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
    explicit Window(const QSurfaceFormat& format) : WindowQt(format) {}

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
        auto* image = new QImage(path + "/../../data/llassetgen-rendering/testfontatlas_DT.png");

        // mirrored: Qt flips images after
        // loading; meant as convenience, but
        // we need it to flip back here.
        auto imageFormatted = image->convertToFormat(QImage::Format_RGBA8888).mirrored(false, true);
        auto imageData = imageFormatted.bits();

        texture = globjects::Texture::createDefault(GL_TEXTURE_2D);
        float imageW = image->width();
        float imageH = image->height();
        texture->image2D(0, GL_RGBA8, imageW, imageH, 0, GL_BGRA, GL_UNSIGNED_BYTE, imageData);
        // TODO: Willy told me that green and blue channels are swapped, that's why GL_BGRA is used here; we also might
        // ignore this, since we use black&white image data here?

        cornerBuffer = globjects::Buffer::create();
        textureBuffer = globjects::Buffer::create();
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

        float quadW = 1.f;
        float quadH = quadW * imageH / imageW;

        cornerBuffer->setData(std::array<glm::vec2, 4>{{glm::vec2(0, 0), glm::vec2(quadW, 0), glm::vec2(0, quadH),
                                                        glm::vec2(quadW, quadH)}},
                              GL_STATIC_DRAW);

        vao->binding(0)->setAttribute(0);
        vao->binding(0)->setBuffer(cornerBuffer.get(), 0, sizeof(glm::vec2));
        vao->binding(0)->setFormat(2, GL_FLOAT);
        vao->enable(0);

        textureBuffer->setData(
            std::array<glm::vec2, 4>{{glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(0, 1), glm::vec2(1, 1)}},
            GL_STATIC_DRAW);

        vao->binding(1)->setAttribute(1);
        vao->binding(1)->setBuffer(textureBuffer.get(), 0, sizeof(glm::vec2));
        vao->binding(1)->setFormat(2, GL_FLOAT);
        vao->enable(1);

        program->setUniform("modelView", transform3D);
        program->setUniform("projection", projection);

        program->setUniform("fontColor", fontColor);
        program->setUniform("glyphs", samplerIndex);
        program->setUniform("showDistanceField", showDistanceField);
        program->setUniform("superSampling", superSampling);
    }

    virtual void deinitializeGL() override {
        texture.reset(nullptr);

        cornerBuffer.reset(nullptr);
        textureBuffer.reset(nullptr);
        program.reset(nullptr);
        vertexShaderSource.reset(nullptr);
        vertexShaderTemplate.reset(nullptr);
        vertexShader.reset(nullptr);
        fragmentShaderSource.reset(nullptr);
        fragmentShaderTemplate.reset(nullptr);
        fragmentShader.reset(nullptr);
        vao.reset(nullptr);
    }

    virtual void resizeGL(QResizeEvent* event) override {
        float w = event->size().width();
        float h = event->size().height();
        glViewport(0, 0, w, h);
        projection = glm::perspective(45.f, w / h, 0.0001f, 100.f);
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

        program->setUniform("modelView", transform3D);
        program->setUniform("projection", projection);

        program->setUniform("fontColor", fontColor);
        // uncomment if you need to change the index for that texture
        // program->setUniform("glyphs", samplerIndex);
        program->setUniform("showDistanceField", showDistanceField);
        program->setUniform("superSampling", superSampling);

        vao->drawArrays(GL_TRIANGLE_STRIP, 0, 4);
        program->release();

        if (texture) {
            texture->unbindActive(samplerIndex);
        }

        glDisable(GL_BLEND);
    }

    virtual void keyPressEvent(QKeyEvent* event) override {
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

    virtual void mousePressEvent(QMouseEvent* event) override {
        makeCurrent();

        lastMousePos.x = event->x();
        lastMousePos.y = event->y();

        if (event->button() == Qt::LeftButton) {
            isPanning = true;
        } else if (event->button() == Qt::RightButton) {
            isRotating = true;
        }

        doneCurrent();
    }

    virtual void mouseMoveEvent(QMouseEvent* event) override {
        // early return
        if (!isPanning && !isRotating) return;

        auto speed = 0.005f;  // magic.

        makeCurrent();

        if ((event->buttons() & Qt::LeftButton) && isPanning) {
            auto deltaX = (event->x() - lastMousePos.x) * speed;
            auto deltaY = (event->y() - lastMousePos.y) * speed * -1;  // minus one because of Qt widget positions.

            transform3D = glm::translate(transform3D, glm::vec3(deltaX, deltaY, 0));

        } else if ((event->buttons() & Qt::RightButton) && isRotating) {
            auto deltaX = (event->x() - lastMousePos.x) * speed;
            auto deltaY = (event->y() - lastMousePos.y) * speed;

            transform3D = glm::rotate(transform3D, deltaX, glm::vec3(0, 1, 0));
            transform3D = glm::rotate(transform3D, deltaY, glm::vec3(1, 0, 0));
            // What's for rotation around z? maybe some GUI Elements?
        }

        lastMousePos.x = event->x();
        lastMousePos.y = event->y();

        paint();

        doneCurrent();
    }

    virtual void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && isPanning) {
            isPanning = false;
        } else if (event->button() == Qt::RightButton && isRotating) {
            isRotating = false;
        }
    }

    virtual void wheelEvent(QWheelEvent* event) override {
        // zooming

        auto speed = 0.001f;  // magic.

        auto d = event->delta() * speed;

        transform3D = glm::translate(transform3D, glm::vec3(0, 0, d));

        paint();
    }

    void applyColorChange(float& color, QString value) {
        color = value.toInt() / 255.f;
        paint();
    }

   public slots:
    virtual void backgroundColorRChanged(QString value) override { applyColorChange(backgroundColor.r, value); }

    virtual void backgroundColorGChanged(QString value) override { applyColorChange(backgroundColor.g, value); }

    virtual void backgroundColorBChanged(QString value) override { applyColorChange(backgroundColor.b, value); }

    virtual void fontColorRChanged(QString value) override { applyColorChange(fontColor.r, value); }

    virtual void fontColorGChanged(QString value) override { applyColorChange(fontColor.g, value); }

    virtual void fontColorBChanged(QString value) override { applyColorChange(fontColor.b, value); }

    virtual void resetTransform3D() override {
        transform3D = glm::mat4();  // set identity
        paint();
    }

    virtual void superSamplingChanged(int index) override {
        superSampling = (uint)index;
        paint();
    }

    virtual void toggleDistanceField(bool activated) override {
        showDistanceField = activated;
        paint();
    }

   protected:
    std::unique_ptr<globjects::Buffer> cornerBuffer;
    std::unique_ptr<globjects::Buffer> textureBuffer;
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
    glm::mat4 transform3D = glm::mat4();
    glm::mat4 projection = glm::perspective(45.f, 1.f, 0.0001f, 100.f);
    uint superSampling = 0;
    bool showDistanceField = false;

    bool isPanning = false;
    bool isRotating = false;
    glm::vec2 lastMousePos = glm::vec2();
};

void prepareColorInput(QLineEdit* input, const QString placeholder) {
    auto colorValidator = new QIntValidator(0, 255);
    input->setValidator(colorValidator);
    input->setPlaceholderText(placeholder);
    input->setMaximumWidth(45);
};

/* This function creates GUI elements and connects them to their correspondent functions using Qt signal-slot.
 * A QApplication offers a window, that must not be destroyed to make QApplication work properly.
 */
void setupGUI(QMainWindow* window) {
    // from globjects
    QSurfaceFormat format;
#ifdef __APPLE__
    format.setVersion(4, 1);  // ToDo: which version is supported on macOS?
    format.setProfile(QSurfaceFormat::CoreProfile);
#else
    format.setVersion(4, 3);
#endif
    format.setDepthBufferSize(16);

    Window* glwindow = new Window(format);

    window->setMinimumSize(640, 480);
    window->setWindowTitle("Open Font Asset Generator");

    // FONT COLOR
    auto fontColorGroupBox = new QGroupBox("Font Color");
    fontColorGroupBox->setMaximumHeight(150);
    auto fontColorLayout = new QFormLayout();

    fontColorGroupBox->setLayout(fontColorLayout);

    // font Color RED
    auto* fontR = new QLineEdit();
    prepareColorInput(fontR, "0");
    QObject::connect(fontR, SIGNAL(textEdited(QString)), glwindow, SLOT(fontColorRChanged(QString)));
    fontColorLayout->addRow("R:", fontR);

    // font Color GREEN
    auto* fontG = new QLineEdit();
    prepareColorInput(fontG, "0");
    QObject::connect(fontG, SIGNAL(textEdited(QString)), glwindow, SLOT(fontColorGChanged(QString)));
    fontColorLayout->addRow("G:", fontG);

    // font Color BLUE
    auto* fontB = new QLineEdit();
    prepareColorInput(fontB, "0");
    QObject::connect(fontB, SIGNAL(textEdited(QString)), glwindow, SLOT(fontColorBChanged(QString)));
    fontColorLayout->addRow("B:", fontB);

    // BACKGROUND COLOR

    auto backgroundColorGroupBox = new QGroupBox("Background Color");
    backgroundColorGroupBox->setMaximumHeight(150);
    auto backgroundColorLayout = new QFormLayout();
    backgroundColorGroupBox->setLayout(backgroundColorLayout);

    // Background Color RED
    auto* backgroundR = new QLineEdit();
    prepareColorInput(backgroundR, "255");
    QObject::connect(backgroundR, SIGNAL(textEdited(QString)), glwindow, SLOT(backgroundColorRChanged(QString)));
    backgroundColorLayout->addRow("R:", backgroundR);

    // Background Color GREEN
    auto* backgroundG = new QLineEdit();
    prepareColorInput(backgroundG, "255");
    QObject::connect(backgroundG, SIGNAL(textEdited(QString)), glwindow, SLOT(backgroundColorGChanged(QString)));
    backgroundColorLayout->addRow("G:", backgroundG);

    // Background Color BLUE
    auto* backgroundB = new QLineEdit();
    prepareColorInput(backgroundB, "255");
    QObject::connect(backgroundB, SIGNAL(textEdited(QString)), glwindow, SLOT(backgroundColorBChanged(QString)));
    backgroundColorLayout->addRow("B:", backgroundB);

    // MISCELLANEOUS GUI

    auto miscGroupBox = new QGroupBox("Miscellaneous");
    miscGroupBox->setMaximumHeight(150);
    auto miscLayout = new QFormLayout();
    miscGroupBox->setLayout(miscLayout);

    // reset transform 3D to inital state
    auto* resetButton = new QPushButton("Reset");
    resetButton->setMaximumWidth(90);
    QObject::connect(resetButton, SIGNAL(clicked()), glwindow, SLOT(resetTransform3D()));
    miscLayout->addRow("Reset View", resetButton);

    // switch between viewing the rendered glyphs and the underlying distance field
    auto* switchRenderingButton = new QPushButton("Distance Field");
    switchRenderingButton->setCheckable(true);
    switchRenderingButton->setMaximumWidth(90);
    QObject::connect(switchRenderingButton, SIGNAL(toggled(bool)), glwindow, SLOT(toggleDistanceField(bool)));
    miscLayout->addRow("Switch Rendering", switchRenderingButton);

    // Supersampling
    auto* ssComboBox = new QComboBox();
    // item order is important, their index is used in fragment shader
    ssComboBox->addItem("None");
    ssComboBox->addItem("1x3");
    ssComboBox->addItem("2x4");
    ssComboBox->addItem("2x2 rotated grid");
    ssComboBox->addItem("Quincunx");
    ssComboBox->addItem("8 Rooks");
    ssComboBox->addItem("3x3");
    ssComboBox->addItem("4x4");

    QObject::connect(ssComboBox, SIGNAL(currentIndexChanged(int)), glwindow, SLOT(superSamplingChanged(int)));
    miscLayout->addRow("Super Sampling", ssComboBox);

    // gather all parameters into one layout (separately from the gl window)
    auto* guiLayout = new QBoxLayout(QBoxLayout::LeftToRight);
    guiLayout->addWidget(backgroundColorGroupBox, 0, Qt::AlignLeft);
    guiLayout->addWidget(fontColorGroupBox, 0, Qt::AlignLeft);
    guiLayout->addWidget(miscGroupBox, 0, Qt::AlignLeft);

    auto* mainLayout = new QBoxLayout(QBoxLayout::TopToBottom);
    mainLayout->addLayout(guiLayout, 0);
    mainLayout->addWidget(QWidget::createWindowContainer(glwindow));

    // since window already has a special layout, we have to put our layout on a widget
    // and then set it as central widget
    auto central = new QWidget();
    central->setLayout(mainLayout);

    window->setCentralWidget(central);
}

int main(int argc, char** argv) {
    llassetgen::init();

    std::unique_ptr<llassetgen::DistanceTransform> dt(new llassetgen::DeadReckoning());

    QApplication app(argc, argv);

    auto path = app.applicationDirPath();
    dt->importPng(path.toStdString() + "/../../data/llassetgen-rendering/testfontatlas.png");
    dt->transform();
    dt->exportPng(path.toStdString() + "/../../data/llassetgen-rendering/testfontatlasDT.png", -20, 50, 8);
    // TODO: don't export, but use as texture directly
    // TODO: exported png is corrupted, wait for update/fix

    QMainWindow* window = new QMainWindow();
    setupGUI(window);
    window->show();

    return app.exec();
}
