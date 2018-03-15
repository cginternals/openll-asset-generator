#include "FontRenderer.h"
#ifdef __unix__
#include <fontconfig/fontconfig.h>
#elif _WIN32
#define NOMINMAX
#include <windows.h>
#include <wingdi.h>
#endif

using namespace llassetgen;

FontRenderer FontRenderer::fromPath(const std::string& fontPath) {
    FontRenderer renderer{};
    FT_Error err = FT_New_Face(freetype, fontPath.c_str(), 0, &renderer.fontFace);
    if (err) {
        throw std::runtime_error("Font could not be loaded");
    }
    return renderer;
}

FontRenderer FontRenderer::fromName(const std::string& fontName) {
#ifdef __unix__
    std::string fontPath;
    if (!findFontPath(fontName, fontPath)) {
        throw std::runtime_error("Font not found");
    }
    return FontRenderer::fromPath(fontPath);
#elif _WIN32
    FontRenderer renderer;
    if (!renderer.getFontData(fontName)) {
        throw std::runtime_error("font not found");
    }

    FT_Error err = FT_New_Memory_Face(freetype, &renderer.fontData[0], renderer.fontData.size(), 0, &renderer.fontFace);
    if (err) {
        throw std::runtime_error("font could not be loaded");
    }
    return renderer;
#endif
}

#ifdef __unix__
bool FontRenderer::findFontPath(const std::string& fontName, std::string& fontPath) {
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
bool FontRenderer::getFontData(const std::string& fontName) {
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

std::unique_ptr<Image> FontRenderer::renderGlyph(unsigned long glyph, unsigned int size) {
    FT_Error err;
    err = FT_Set_Pixel_Sizes(fontFace, 0, static_cast<FT_UInt>(size));
    if (err) {
        throw std::runtime_error("could not set font size");
    }

    FT_UInt charIndex = FT_Get_Char_Index(fontFace, static_cast<FT_ULong>(glyph));
    err = FT_Load_Glyph(fontFace, charIndex, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO);

    if (err || fontFace->glyph->bitmap.buffer == nullptr) {
        throw std::runtime_error("glyph could not be rendered");
    }

    FT_Bitmap& bitmap = fontFace->glyph->bitmap;
    auto img = std::unique_ptr<Image>(new Image(bitmap.width, bitmap.rows, 1));
    img->load(bitmap);
    return img;
}
