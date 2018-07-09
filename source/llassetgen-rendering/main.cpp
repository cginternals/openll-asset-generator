#include <algorithm>
#include <iostream>
#include <string>

#include <ft2build.h>  // NOLINT include order required by freetype
#include FT_FREETYPE_H

#include <QtGlobal>

#include <QApplication>
#include <QBoxLayout>
#include <QComboBox>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QResizeEvent>
#include <QStandardPaths>
#include <QValidator>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include <glbinding-aux/ContextInfo.h>
#include <glbinding/Version.h>
#include <glbinding/gl/gl.h>

#include <glbinding-aux/types_to_string.h>

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
        globjects::init([this](const char * name) {
            return getProcAddress(name);
        });

        std::cout << std::endl
                  << "OpenGL Version:  " << glbinding::aux::ContextInfo::version() << std::endl
                  << "OpenGL Vendor:   " << glbinding::aux::ContextInfo::vendor() << std::endl
                  << "OpenGL Renderer: " << glbinding::aux::ContextInfo::renderer() << std::endl
                  << std::endl;

        globjects::DebugMessage::enable();

#ifdef __APPLE__
        globjects::Shader::clearGlobalReplacements();
        globjects::Shader::globalReplace("#version 140", "#version 150");

        globjects::debug() << "Using global OS X shader replacement '#version 140' -> '#version 150'" << std::endl;
#endif

        std::string dataPath = "./data/llassetgen-rendering/";

#if QT_VERSION >= 0x050400
        outDirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
        outDirPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#endif

        QDir dir(QDir::root());
        bool success = dir.mkpath(outDirPath);
        std::cout << "Created directory: " + outDirPath.toStdString() + ", success: " << success << std::endl;

        // get glyph atlas
        calculateDistanceField();

        cornerBuffer = globjects::Buffer::create();
        textureBuffer = globjects::Buffer::create();
        program = globjects::Program::create();
        vao = globjects::VertexArray::create();

        loadDistanceField();
        /* already set in loadDistanceField()
        vao->binding(0)->setAttribute(0);
        vao->binding(0)->setBuffer(cornerBuffer.get(), 0, sizeof(glm::vec2));
        vao->binding(0)->setFormat(2, GL_FLOAT);
        vao->enable(0);
        */

        vertexShaderSource = globjects::Shader::sourceFromFile(dataPath + "shader.vert");
        vertexShaderTemplate = globjects::Shader::applyGlobalReplacements(vertexShaderSource.get());
        vertexShader = globjects::Shader::create(GL_VERTEX_SHADER, vertexShaderTemplate.get());

        fragmentShaderSource = globjects::Shader::sourceFromFile(dataPath + "shader.frag");
        fragmentShaderTemplate = globjects::Shader::applyGlobalReplacements(fragmentShaderSource.get());
        fragmentShader = globjects::Shader::create(GL_FRAGMENT_SHADER, fragmentShaderTemplate.get());

        program->attach(vertexShader.get(), fragmentShader.get());

        textureBuffer->setData(std::array<glm::vec2, 4>{{{0, 0}, {1, 0}, {0, 1}, {1, 1}}}, GL_STATIC_DRAW);

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
        program->setUniform("threshold", dtThreshold);
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
        program->setUniform("threshold", dtThreshold);

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
        lastMousePos.x = event->x();
        lastMousePos.y = event->y();

        if (event->button() == Qt::LeftButton) {
            isPanning = true;
        } else if (event->button() == Qt::RightButton) {
            isRotating = true;
        }
    }

    virtual void mouseMoveEvent(QMouseEvent* event) override {
        // early return
        if (!isPanning && !isRotating) return;

        auto speed = 0.005f;  // magic.

        if ((event->buttons() & Qt::LeftButton) && isPanning) {
            auto deltaX = (event->x() - lastMousePos.x) * speed;
            auto deltaY = (event->y() - lastMousePos.y) * speed * -1;  // minus one because of Qt widget positions.

            transform3D = glm::translate(transform3D, glm::vec3(deltaX, deltaY, 0));

        } else if ((event->buttons() & Qt::RightButton) && isRotating) {
            auto deltaX = (event->x() - lastMousePos.x) * speed;
            auto deltaY = (event->y() - lastMousePos.y) * speed;

            transform3D = glm::rotate(transform3D, deltaX, glm::vec3(0, 1, 0));
            transform3D = glm::rotate(transform3D, deltaY, glm::vec3(1, 0, 0));
            // What's for rotation around z? Maybe some GUI Elements?
            // Since this navigation is not well elaborated but rudimentary, a rotation around z is achieved
            // by moving the mouse (counter)-clockwise while holding right button pressed. This is an ugly
            // navigation, but it's "okay enough" for our purpose.
        }

        lastMousePos.x = event->x();
        lastMousePos.y = event->y();

        paint();
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

    virtual void dtAlgorithmChanged(int index) override { dtAlgorithm = index; }

    virtual void packingAlgoChanged(int index) override { packingAlgorithm = index; }

    virtual void dtThresholdChanged(QString value) override {
        dtThreshold = value.toFloat();
        paint();
    }

    virtual void packingSizeChanged(int index) override {
        std::cout << "change downsampling" << std::endl;
        downSampling = index;
    }

    virtual void resetTransform3D() override {
        transform3D = glm::mat4();  // set identity
        paint();
    }

    virtual void triggerNewDT() override {
        makeCurrent();

        calculateDistanceField();
        loadDistanceField();

        doneCurrent();

        paint();
    }

    virtual void superSamplingChanged(int index) override {
        superSampling = static_cast<unsigned int>(index);
        paint();
    }

    virtual void toggleDistanceField(bool activated) override {
        showDistanceField = activated;
        paint();
    }

    virtual void fontNameChanged(QString value) override {
        std::cout << "fontNameChanged: " + value.toStdString() << std::endl;
        fontName = value.toStdString();
    }

    virtual void fontSizeChanged(QString value) override {
        std::cout << "fontSizeChanged: " + value.toStdString() << std::endl;
        fontSize = value.toInt();
    }

    virtual void drBlackChanged(QString value) override {
        std::cout << "drBlack Changed: " + value.toStdString() << std::endl;
        drBlack = value.toInt();
    }

    virtual void drWhiteChanged(QString value) override {
        std::cout << "drWhite Changed: " + value.toStdString() << std::endl;
        drWhite = value.toInt();
    }

    virtual void paddingChanged(QString value) override { padding = value.toInt(); }

    virtual void exportGlyphAtlas() override {
        std::cout << "TODO EXPORT: atlas and fnt-file is exported automatically when changes applied, but path for "
                     "output is hard-coded:"
                  << outDirPath.toStdString() <<  std::endl << "Use CLI-app for custom path." << std::endl;
        // TODO export dialog: ask user for path
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
    unsigned int superSampling = 0;
    bool showDistanceField = false;
    float dtThreshold = 0.5;
    int dtAlgorithm = 0;
    int packingAlgorithm = 0;
    int downSampling = 0;
#ifdef SYSTEM_WINDOWS
    std::string fontName = "Verdana";
#elif defined(SYSTEM_DARWIN)
    std::string fontName = "Verdana";
#else
    std::string fontName = "Ubuntu";
#endif
    unsigned int fontSize = 512;
    int drBlack = -100;
    int drWhite = 100;
    int padding = 4;

    bool isPanning = false;
    bool isRotating = false;
    glm::vec2 lastMousePos = glm::vec2();

    QString outDirPath = "";

    void calculateDistanceField() {
        auto outImagePath = (outDirPath + "/outputDT.png").toStdString();
        auto outFntPath = (outDirPath + "/outputFNT.fnt").toStdString();

        try {
            llassetgen::FontFinder fontFinder = llassetgen::FontFinder::fromName(fontName);

            std::set<unsigned long> glyphSet;

            // a custom preset using unicode
            constexpr char16_t preset20180319[] = {
                u'\x01',   u'\x03',   u'\x07',   u'\x11',   u' ',      u'!',      u'#',      u'$',      u'%',      u'&',
                u'\x27',   u'(',      u')',      u'*',      u'+',      u',',      u'-',      u'.',      u'/',      u'0',
                u'1',      u'2',      u'3',      u'4',      u'5',      u'6',      u'7',      u'8',      u'9',      u':',
                u';',      u'<',      u'=',      u'>',      u'?',      u'@',      u'A',      u'B',      u'C',      u'D',
                u'E',      u'F',      u'G',      u'H',      u'I',      u'J',      u'K',      u'L',      u'M',      u'N',
                u'O',      u'P',      u'Q',      u'R',      u'S',      u'T',      u'U',      u'V',      u'W',      u'X',
                u'Y',      u'Z',      u'[',      u'\\',     u']',      u'^',      u'_',      u'`',      u'a',      u'b',
                u'c',      u'd',      u'e',      u'f',      u'g',      u'h',      u'i',      u'j',      u'k',      u'l',
                u'm',      u'n',      u'o',      u'p',      u'q',      u'r',      u's',      u't',      u'u',      u'v',
                u'w',      u'x',      u'y',      u'z',      u'{',      u'}',      u'~',      u'\x7f',   u'\x84',   u'\x9c',
                u'\xa4',   u'\xb0',   u'\xb6',   u'\xbc',   u'\xc3',   u'\xc4',   u'\xd3',   u'\xd6',   u'\xdc',   u'\xdf',
                u'\xe0',   u'\xe1',   u'\xe4',   u'\xf0',   u'\xf3',   u'\xf6',   u'\xfc',   u'\u0178', u'\u0394', u'\u03ae',
                u'\u03b1', u'\u03b9', u'\u03bc', u'\u03bf', u'\u03c1', u'\u03c4', u'\u03c6', u'\u03c9', u'\u041a', u'\u0430',
                u'\u0433', u'\u0438', u'\u043d', u'\u043e', u'\u0440', u'\u0442', u'\u0443', u'\u0444', u'\u2013', u'\u20ac',
                u'\u2192', u'\u3001', u'\u30a3', u'\u30ae', u'\u30b3', u'\u30bf', u'\u30d5', u'\u30e5', u'\u30ec', u'\u30f3',
                u'\u30fc', u'\u4e09', u'\u4e2a', u'\u4e2d', u'\u4e3a', u'\u4e3b', u'\u4e49', u'\u4e92', u'\u4ea4', u'\u4ea7',
                u'\u4eba', u'\u4ecb', u'\u4ed6', u'\u4ee3', u'\u4ef6', u'\u4fdd', u'\u4fe1', u'\u503a', u'\u505a', u'\u516c',
                u'\u5176', u'\u5177', u'\u51ed', u'\u5230', u'\u5238', u'\u526f', u'\u52a0', u'\u5305', u'\u5316', u'\u5355',
                u'\u5386', u'\u539f', u'\u53d1', u'\u53f0', u'\u53f2', u'\u54c1', u'\u552e', u'\u56e0', u'\u573a', u'\u578b',
                u'\u5904', u'\u5916', u'\u5931', u'\u5b57', u'\u5b9a', u'\u5b9e', u'\u5de5', u'\u5df2', u'\u5e02', u'\u5e73',
                u'\u5f00', u'\u5f52', u'\u5f55', u'\u6027', u'\u606f', u'\u627f', u'\u6295', u'\u62bc', u'\u62cd', u'\u6362',
                u'\u636e', u'\u63d0', u'\u6444', u'\u6536', u'\u6570', u'\u6587', u'\u65b0', u'\u65e5', u'\u6613', u'\u672c',
                u'\u677f', u'\u6784', u'\u679c', u'\u67e5', u'\u6848', u'\u6863', u'\u6b3e', u'\u6bb5', u'\u6d4b', u'\u6d88',
                u'\u6dfb', u'\u6e20', u'\u6e70', u'\u6e90', u'\u7406', u'\u7528', u'\u7533', u'\u754c', u'\u767b', u'\u7684',
                u'\u76ee', u'\u7801', u'\u7968', u'\u7a7a', u'\u7acb', u'\u7ed3', u'\u80a1', u'\u8272', u'\u8425', u'\u884c',
                u'\u8868', u'\u88fd', u'\u8907', u'\u89c8', u'\u89d2', u'\u8bc1', u'\u8bd5', u'\u8be2', u'\u8bf7', u'\u8c03',
                u'\u8d25', u'\u8d26', u'\u8d28', u'\u8d37', u'\u8d39', u'\u8f6c', u'\u8f91', u'\u903b', u'\u9053', u'\u9353',
                u'\u94f6', u'\u9500', u'\u9636', u'\u9644', u'\u9762', u'\u9875', u'\u9879', u'\u9884', u'\ue21b', u'\uff0d',
                u'\uff0f', u'\ufffd' };

            const char16_t* s = preset20180319;
            while (*s) {
                glyphSet.insert(static_cast<unsigned long>(*s++));
            }

            // TODO: GUI element for this
            unsigned int downsamplingRatio = 4;

            std::vector<llassetgen::Image> glyphImages =
                fontFinder.renderGlyphs(glyphSet, fontSize, padding, downsamplingRatio);

            std::vector<llassetgen::Vec2<size_t>> imageSizes(glyphImages.size());
            std::transform(
                glyphImages.begin(), glyphImages.end(), imageSizes.begin(),
                [downsamplingRatio](const llassetgen::Image& img) { return img.getSize() / downsamplingRatio; });

            llassetgen::Packing pack;
            switch (packingAlgorithm) {
                case 0: {
                    pack = llassetgen::shelfPackAtlas(imageSizes.begin(), imageSizes.end(), false);
                    break;
                }
                default:
                case 1: {
                    pack = llassetgen::maxRectsPackAtlas(imageSizes.begin(), imageSizes.end(), false);
                    break;
                }
            }

            switch (dtAlgorithm) {
                case 0: {
                    llassetgen::Image atlas = llassetgen::distanceFieldAtlas(
                        glyphImages.begin(), glyphImages.end(), pack,
                        [](llassetgen::Image& input, llassetgen::Image& output) {
                            llassetgen::DeadReckoning(input, output).transform();
                        },
                        [](llassetgen::Image& input, llassetgen::Image& output) {
                            input.averageDownsampling<llassetgen::DistanceTransform::OutputType>(output);
                        });
                    atlas.exportPng<llassetgen::DistanceTransform::OutputType>(outImagePath, drWhite, drBlack);

                    break;
                }
                default:
                case 1: {
                    llassetgen::Image atlas = llassetgen::distanceFieldAtlas(
                        glyphImages.begin(), glyphImages.end(), pack,
                        [](llassetgen::Image& input, llassetgen::Image& output) {
                            llassetgen::ParabolaEnvelope(input, output).transform();
                        },
                        [](llassetgen::Image& input, llassetgen::Image& output) {
                            input.averageDownsampling<llassetgen::DistanceTransform::OutputType>(output);
                        });
                    atlas.exportPng<llassetgen::DistanceTransform::OutputType>(outImagePath, drWhite, drBlack);

                    break;
                }
            }

            // export fnt file
            llassetgen::FntWriter writer{fontFinder.fontFace, fontName, fontSize, 1, false};
            writer.setAtlasProperties(pack.atlasSize, fontSize);
            writer.readFont(glyphSet.begin(), glyphSet.end());
            auto gIt = glyphSet.begin();
            for (auto rectIt = pack.rects.begin(); rectIt < pack.rects.end(); gIt++, rectIt++) {
                FT_UInt charIndex = FT_Get_Char_Index(fontFinder.fontFace, static_cast<FT_ULong>(*gIt));
                writer.setCharInfo(charIndex, *rectIt, {0, 0});
            }
            writer.saveFnt(outFntPath);

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void loadDistanceField() {
        auto* image = new QImage(outDirPath + "/outputDT.png");

        if (image->isNull()) {
            std::cout << "Image NOT loaded successfully." << std::endl;
        }

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

        float quadW = 1.f;
        float quadH = quadW * imageH / imageW;

        cornerBuffer->setData(std::array<glm::vec2, 4>{{{0, 0}, {quadW, 0}, {0, quadH}, {quadW, quadH}}},
                              GL_STATIC_DRAW);

        vao->binding(0)->setAttribute(0);
        vao->binding(0)->setBuffer(cornerBuffer.get(), 0, sizeof(glm::vec2));
        vao->binding(0)->setFormat(2, GL_FLOAT);
        vao->enable(0);

        program->setUniform("glyphs", samplerIndex);
    }
};

void prepareColorInput(QLineEdit* input, const QString placeholder) {
    auto* colorValidator = new QIntValidator(0, 255);
    input->setValidator(colorValidator);
    input->setPlaceholderText(placeholder);
    input->setMaximumWidth(28);
}

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

    int groupboxMaxHeight = 160;

    // COLOR OPTIONS

    auto* colorGroupBox = new QGroupBox("Rendering Colors RGB");
    colorGroupBox->setMaximumHeight(groupboxMaxHeight);
    auto* colorLayout = new QFormLayout();

    colorGroupBox->setLayout(colorLayout);

    // FONT COLOR

    auto* fontColorLayout = new QHBoxLayout();
    colorLayout->addRow("Font:", fontColorLayout);

    // font Color RED
    auto* fontR = new QLineEdit();
    prepareColorInput(fontR, "0");
    QObject::connect(fontR, SIGNAL(textEdited(QString)), glwindow, SLOT(fontColorRChanged(QString)));
    fontColorLayout->addWidget(fontR);

    // font Color GREEN
    auto* fontG = new QLineEdit();
    prepareColorInput(fontG, "0");
    QObject::connect(fontG, SIGNAL(textEdited(QString)), glwindow, SLOT(fontColorGChanged(QString)));
    fontColorLayout->addWidget(fontG);

    // font Color BLUE
    auto* fontB = new QLineEdit();
    prepareColorInput(fontB, "0");
    QObject::connect(fontB, SIGNAL(textEdited(QString)), glwindow, SLOT(fontColorBChanged(QString)));
    fontColorLayout->addWidget(fontB);

    // BACKGROUND COLOR

    auto* backgroundColorLayout = new QHBoxLayout();
    colorLayout->addRow("Back:", backgroundColorLayout);

    // Background Color RED
    auto* backgroundR = new QLineEdit();
    prepareColorInput(backgroundR, "255");
    QObject::connect(backgroundR, SIGNAL(textEdited(QString)), glwindow, SLOT(backgroundColorRChanged(QString)));
    backgroundColorLayout->addWidget(backgroundR);

    // Background Color GREEN
    auto* backgroundG = new QLineEdit();
    prepareColorInput(backgroundG, "255");
    QObject::connect(backgroundG, SIGNAL(textEdited(QString)), glwindow, SLOT(backgroundColorGChanged(QString)));
    backgroundColorLayout->addWidget(backgroundG);

    // Background Color BLUE
    auto* backgroundB = new QLineEdit();
    prepareColorInput(backgroundB, "255");
    QObject::connect(backgroundB, SIGNAL(textEdited(QString)), glwindow, SLOT(backgroundColorBChanged(QString)));
    backgroundColorLayout->addWidget(backgroundB);

    // DISTANCE FIELD CREATION OPTIONS

    auto* dfGroupBox = new QGroupBox("Distance Field Options");
    dfGroupBox->setMaximumHeight(groupboxMaxHeight);
    auto* dfLayout = new QHBoxLayout();
    dfGroupBox->setLayout(dfLayout);

    // ATLAS CREATION OPTIONS

    auto* acLayout = new QFormLayout();
    dfLayout->addLayout(acLayout);

    // typeface of font
    auto* fontNameLE = new QLineEdit();
    fontNameLE->setPlaceholderText("Verdana");
    fontNameLE->setMaximumWidth(45);
    QObject::connect(fontNameLE, SIGNAL(textEdited(QString)), glwindow, SLOT(fontNameChanged(QString)));
    acLayout->addRow("Font Name:", fontNameLE);

    // choose packing algorithm
    auto* packComboBox = new QComboBox();
    // item order is important
    packComboBox->addItem("Shelf");
    packComboBox->addItem("Max Rects");
    QObject::connect(packComboBox, SIGNAL(currentIndexChanged(int)), glwindow, SLOT(packingAlgoChanged(int)));
    acLayout->addRow("Packing:", packComboBox);

    // original font size for distance field rendering
    auto* fontSizeLE = new QLineEdit();
    auto* fsv = new QIntValidator();
    fsv->setBottom(1);
    fontSizeLE->setValidator(fsv);
    fontSizeLE->setPlaceholderText("512");
    fontSizeLE->setMaximumWidth(45);
    QObject::connect(fontSizeLE, SIGNAL(textEdited(QString)), glwindow, SLOT(fontSizeChanged(QString)));
    acLayout->addRow("Original Font Size:", fontSizeLE);

    // DISTANCE TRANSFORM OPTIONS

    auto* dtLayout = new QFormLayout();
    dfLayout->addLayout(dtLayout);

    // switch between different distance field arithms
    auto* dtComboBox = new QComboBox();
    // item order is important
    dtComboBox->addItem("Dead Reckoning");
    dtComboBox->addItem("Parabola Envelope");
    QObject::connect(dtComboBox, SIGNAL(currentIndexChanged(int)), glwindow, SLOT(dtAlgorithmChanged(int)));
    dtLayout->addRow("Algorithm:", dtComboBox);

    // dynamic range for distance field rendering
    auto* drLayout = new QHBoxLayout();
    auto* drv = new QIntValidator();

    auto* drBlack = new QLineEdit();
    drBlack->setValidator(drv);
    drBlack->setPlaceholderText("-100");
    drBlack->setMaximumWidth(38);
    QObject::connect(drBlack, SIGNAL(textEdited(QString)), glwindow, SLOT(drBlackChanged(QString)));
    drLayout->addWidget(new QLabel("["));
    drLayout->addWidget(drBlack);
    drLayout->addWidget(new QLabel(","));

    auto* drWhite = new QLineEdit();
    drWhite->setValidator(drv);
    drWhite->setPlaceholderText("100");
    drWhite->setMaximumWidth(38);
    QObject::connect(drWhite, SIGNAL(textEdited(QString)), glwindow, SLOT(drWhiteChanged(QString)));
    drLayout->addWidget(drWhite);
    drLayout->addWidget(new QLabel("]"));

    dtLayout->addRow("Dynamic Range:", drLayout);

    auto* paddingEdit = new QLineEdit();
    paddingEdit->setValidator(drv);
    paddingEdit->setPlaceholderText("4");
    paddingEdit->setMaximumWidth(38);
    QObject::connect(paddingEdit, SIGNAL(textEdited(QString)), glwindow, SLOT(paddingChanged(QString)));
    dtLayout->addRow("Padding:", paddingEdit);

    // packing size (used for downsampling)
    auto* psComboBox = new QComboBox();
    // item order is important
    psComboBox->addItem("64");
    psComboBox->addItem("128");
    psComboBox->addItem("265");
    psComboBox->addItem("512");
    psComboBox->addItem("1024");
    psComboBox->addItem("2048");
    psComboBox->addItem("4096");
    psComboBox->addItem("8192");
    QObject::connect(psComboBox, SIGNAL(currentIndexChanged(int)), glwindow, SLOT(packingSizeChanged(int)));
    // TODO uncomment when downsampling is implemented in llassetgen
    // maybe change from dropdown to float input
    // dtLayout->addRow("Texture size:", psComboBox);

    // trigger distance field creation
    auto* triggerDTButton = new QPushButton("OK");
    triggerDTButton->setMaximumWidth(50);
    QObject::connect(triggerDTButton, SIGNAL(clicked()), glwindow, SLOT(triggerNewDT()));
    acLayout->addRow("Apply options:", triggerDTButton);

    // export distance field
    auto* exportButton = new QPushButton("Export");
    exportButton->setMaximumWidth(90);
    QObject::connect(exportButton, SIGNAL(clicked()), glwindow, SLOT(exportGlyphAtlas()));
    dtLayout->addRow("Export Atlas:", exportButton);

    // RENDERING OPTIONS

    auto* renderingGroupBox = new QGroupBox("Rendering");
    renderingGroupBox->setMaximumHeight(groupboxMaxHeight);
    auto* renderingLayout = new QFormLayout();
    renderingGroupBox->setLayout(renderingLayout);

    // reset transform 3D to inital state
    auto* resetButton = new QPushButton("Reset");
    resetButton->setMaximumWidth(90);
    QObject::connect(resetButton, SIGNAL(clicked()), glwindow, SLOT(resetTransform3D()));
    renderingLayout->addRow("Reset View:", resetButton);

    // threshold for distance field rendering
    auto* dtT = new QLineEdit();
    auto* dv = new QDoubleValidator(0.3, 1, 5);
    dtT->setValidator(dv);
    dtT->setPlaceholderText("0.5");
    dtT->setMaximumWidth(45);
    QObject::connect(dtT, SIGNAL(textEdited(QString)), glwindow, SLOT(dtThresholdChanged(QString)));
    renderingLayout->addRow("Threshold:", dtT);

    // switch between viewing the rendered glyphs and the underlying distance field
    auto* switchRenderingButton = new QPushButton("Distance Field");
    switchRenderingButton->setCheckable(true);
    switchRenderingButton->setMaximumWidth(100);
    QObject::connect(switchRenderingButton, SIGNAL(toggled(bool)), glwindow, SLOT(toggleDistanceField(bool)));
    renderingLayout->addRow("Switch Rendering:", switchRenderingButton);

    // Supersampling
    auto* ssComboBox = new QComboBox();
    ssComboBox->setMaximumWidth(90);
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
    renderingLayout->addRow("Super Sampling:", ssComboBox);

    // gather all parameters into one layout (separately from the gl window)
    auto* guiLayout = new QBoxLayout(QBoxLayout::LeftToRight);
    guiLayout->addWidget(dfGroupBox, 0, Qt::AlignLeft);
    guiLayout->addWidget(renderingGroupBox, 0, Qt::AlignLeft);
    guiLayout->addWidget(colorGroupBox, 0, Qt::AlignLeft);

    auto* mainLayout = new QBoxLayout(QBoxLayout::TopToBottom);
    mainLayout->addLayout(guiLayout, 0);
    mainLayout->addWidget(QWidget::createWindowContainer(glwindow));

    // since window already has a special layout, we have to put our layout on a widget
    // and then set it as central widget
    auto* central = new QWidget();
    central->setLayout(mainLayout);

    window->setCentralWidget(central);
}

int main(int argc, char** argv) {
    llassetgen::init();

    QApplication app(argc, argv);

    QMainWindow* window = new QMainWindow();
    setupGUI(window);
    window->show();

    return app.exec();
}
