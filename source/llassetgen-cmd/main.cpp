#include <iostream>
#include <map>

#include <CLI11.h>
#include <ft2build.h>  // NOLINT include order required by freetype
#include FT_FREETYPE_H

#ifdef __unix__
#include <fontconfig/fontconfig.h>
#elif _WIN32
#include <windows.h>
#include <wingdi.h>
#endif

#include <llassetgen/llassetgen.h>
#include <codecvt>

using namespace llassetgen;

std::map<std::string, std::function<std::unique_ptr<DistanceTransform>(Image&, Image&)>> dtFactory{
    std::make_pair("deadrec",
                   [](Image& input, Image& output) {
                       return std::unique_ptr<DistanceTransform>(new DeadReckoning(input, output));
                   }),
    std::make_pair("parabola",
                   [](Image& input, Image& output) {
                       return std::unique_ptr<DistanceTransform>(new ParabolaEnvelope(input, output));
                   }),
};

std::u32string UTF8toUCS4(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> ucs4conv;
    return ucs4conv.from_bytes(str);
}

std::unique_ptr<Image> renderGlyph(FT_ULong glyph, const FT_Face& face, FT_UInt size) {
    FT_Error err;
    err = FT_Set_Pixel_Sizes(face, 0, size);
    if (err) {
        std::cerr << "Error: could not set font size" << std::endl;
        return nullptr;
    }

    err = FT_Load_Glyph(face, FT_Get_Char_Index(face, glyph), FT_LOAD_RENDER | FT_LOAD_TARGET_MONO);
    if (err || face->glyph->bitmap.buffer == nullptr) {
        std::cerr << "Error: glyph could not be rendered" << std::endl;
        return nullptr;
    }

    auto bitmap = std::make_shared<FT_Bitmap>(face->glyph->bitmap);
    return std::unique_ptr<Image>(new Image(*bitmap));
}

std::unique_ptr<Image> loadGlyph(const std::string& glyphs, const FT_Face& fontFace, int size) {
    std::u32string ucs4Glyph = UTF8toUCS4(glyphs);
    if (ucs4Glyph.length() != 1) {
        std::cerr << "--glyph must be a single character" << std::endl;
        return nullptr;
    }
    return renderGlyph(ucs4Glyph[0], fontFace, static_cast<FT_UInt>(size));
}

bool fontFaceFromPath(const std::string& fontPath, FT_Face& fontFace) {
    FT_Error err = FT_New_Face(freetype, fontPath.c_str(), 0, &fontFace);
    if (err) {
        std::cout << "Error: font could not be loaded" << std::endl;
        return false;
    }
    return true;
}

#ifdef __unix__
bool findFontPath(const std::string& fontName, std::string& fontPath) {
    FcConfig* config = FcInitLoadConfigAndFonts();
    FcPattern* pat = FcNameParse(reinterpret_cast<const FcChar8*>(fontName.c_str()));
    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    FcResult result;
    FcPattern* font = FcFontMatch(config, pat, &result);

    bool found = false;
    if (result == FcResultMatch) {
        FcChar8* file;
        found = FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch;
        if (found) {
            fontPath.assign(reinterpret_cast<char*>(file));
        }
        FcPatternDestroy(font);
    }
    FcPatternDestroy(pat);
    return found;
}
#endif

#if _WIN32
bool getFontData(const std::string& fontName, std::vector<unsigned char>& fontData) {
    bool result = false;

    LOGFONTA lf;
    memset(&lf, 0, sizeof lf);
    strncpy_s(lf.lfFaceName, LF_FACESIZE, fontName.c_str(), fontName.size());
    HFONT font = CreateFontIndirectA(&lf);

    HDC deviceContext = CreateCompatibleDC(nullptr);
    if (deviceContext) {
        SelectObject(deviceContext, font);

        const size_t size = GetFontData(deviceContext, 0, 0, nullptr, 0);
        if (size > 0 && size != GDI_ERROR) {
            auto buffer = new unsigned char[size];
            if (GetFontData(deviceContext, 0, 0, buffer, size) == size) {
                fontData.assign(buffer, buffer + size);
                result = true;
            }
            delete[] buffer;
        }
        DeleteDC(deviceContext);
    }
    return result;
}
#endif

#if __unix__
bool fontFaceFromName(const std::string& fontName, FT_Face& fontFace) {
    std::string fontPath;
    if (!findFontPath(fontName, fontPath)) {
        std::cerr << "Error: font not found" << std::endl;
        return false;
    }
    return fontFaceFromPath(fontPath, fontFace);
#elif _WIN32
    bool fontFaceFromName(const std::string& fontName, FT_Face& fontFace, std::vector<FT_Byte> fontData) {
    if (!getFontData(fontName, fontData)) {
        std::cerr << "Error: font not found" << std::endl;
        return false;
    }

    FT_Error err = FT_New_Memory_Face(freetype, &fontData[0], fontData.size(), 0, &fontFace);
    if (err) {
        std::cout << "Error: font could not be loaded" << std::endl;
        return false;
    }
    return true;
#endif
}

void distField(std::string& algorithm, Image& input, std::string& outPath) {
    Image output = Image(input.getWidth(), input.getHeight(), sizeof(DistanceTransform::OutputType) * 8);
    std::unique_ptr<DistanceTransform> dt = dtFactory[algorithm](input, output);
    dt->transform();
    output.exportPng<DistanceTransform::OutputType>(outPath, -20, 50);
}

int main(int argc, char** argv) {
    llassetgen::init();

    CLI::App app{"OpenLL Font Asset Generator"};
    CLI::App* distfield = app.add_subcommand("distfield");
    CLI::App* atlas = app.add_subcommand("atlas");

    // distfield params
    std::string algorithm = "deadrec";  // default value
    std::set<std::string> algoOptions;
    for (const auto& algo : dtFactory) {
        algoOptions.insert(algo.first);
    }
    distfield->add_set("-a,--algorithm", algorithm, algoOptions);

    std::string glyphs;
    CLI::Option* glyphsOpt = distfield->add_option("-g,--glyph", glyphs);

    std::string fontPath;
    CLI::Option* fontPathOpt = distfield->add_option("--fontpath", fontPath)->check(CLI::ExistingFile);
    fontPathOpt->requires(glyphsOpt);

    std::string fontName;
    CLI::Option* fontNameOpt = distfield->add_option("--fontname,-f", fontName);
    fontNameOpt->requires(glyphsOpt);

    glyphsOpt->requires_one({fontPathOpt, fontNameOpt});

    int fontSize = 128;
    distfield->add_option("-s,--fontsize", fontSize)->requires(glyphsOpt);

    std::string imgPath;
    CLI::Option* imgOpt = distfield->add_option("--image,-i", imgPath)->check(CLI::ExistingFile);
    imgOpt->excludes(glyphsOpt);

    std::string outPath;
    distfield->add_option("outfile", outPath)->required();

    // atlas params
    // TODO

    CLI11_PARSE(app, argc, argv);

    if (app.got_subcommand(distfield)) {
        std::unique_ptr<Image> input;
        if (glyphsOpt->count()) {
            // Example: llassetgen-cmd distfield -a parabola -g G -f Verdana -s 64 glyph.png
            FT_Face fontFace;
            bool faceLoaded;
            if (fontPathOpt->count()) {
                faceLoaded = fontFaceFromPath(fontPath, fontFace);
            } else {
#ifdef _WIN32
                std::vector<FT_Byte> fontData;
                faceLoaded = fontFaceFromName(fontName, fontFace, fontData);
#else
                faceLoaded = fontFaceFromName(fontName, fontFace);
#endif
            }

            if (!faceLoaded) return 2;

            input = loadGlyph(glyphs, fontFace, fontSize);
        } else if (imgOpt->count()) {
            // Example: llassetgen-cmd distfield -a deadrec -i input.png output.png
            input = std::unique_ptr<Image>(new Image(imgPath));
        } else {
            return distfield->exit(CLI::CallForHelp());
        }

        if (!input) return 2;

        distField(algorithm, *input, outPath);
    } else if (app.got_subcommand(atlas)) {
        // TODO
    } else {
        return app.exit(CLI::CallForHelp());
    }

    return 0;
}
