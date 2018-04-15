#if defined(__unix__) || defined(__APPLE__)
    #include <fontconfig/fontconfig.h>
#elif _WIN32
    #define NOMINMAX
    #include <windows.h>
    #include <wingdi.h>
#endif

#include <iostream>

#include <llassetgen/FontFinder.h>


namespace llassetgen {
    FontFinder FontFinder::fromPath(const std::string &fontPath) {
        FontFinder fontFinder{};
        FT_Error err = FT_New_Face(freetype, fontPath.c_str(), 0, &fontFinder.fontFace);
        if (err) {
            throw std::runtime_error("Font could not be loaded");
        }
        return fontFinder;
    }

    FontFinder FontFinder::fromName(const std::string &fontName) {
#if defined(__unix__) || defined(__APPLE__)
        std::string fontPath;
        if (!findFontPath(fontName, fontPath)) {
            throw std::runtime_error("Font not found");
        }
        return FontFinder::fromPath(fontPath);
#elif _WIN32
        FontFinder fontFinder;
        if (!fontFinder.getFontData(fontName)) {
            throw std::runtime_error("font not found");
        }

        FT_Error err = FT_New_Memory_Face(freetype, &fontFinder.fontData[0], fontFinder.fontData.size(), 0, &fontFinder.fontFace);
        if (err) {
            throw std::runtime_error("font could not be loaded");
        }
        return fontFinder;
#endif
    }

#if defined(__unix__) || defined(__APPLE__)
    bool FontFinder::findFontPath(const std::string &fontName, std::string &fontPath) {
        FcConfig *config = FcInitLoadConfigAndFonts();
        FcPattern *pat = FcNameParse(reinterpret_cast<const FcChar8 *>(fontName.c_str()));
        FcConfigSubstitute(config, pat, FcMatchPattern);
        FcDefaultSubstitute(pat);

        FcResult result;
        FcPattern *font = FcFontMatch(config, pat, &result);

        bool found = false;
        if (result == FcResultMatch) {
            FcChar8 *file;
            found = FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch;
            if (found) {
                fontPath.assign(reinterpret_cast<char *>(file));
            }
            FcPatternDestroy(font);
        }
        FcPatternDestroy(pat);
        return found;
    }

#endif

#if _WIN32
    bool FontFinder::getFontData(const std::string& fontName) {
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

    void FontFinder::setFontSize(int size) {
        FT_Error err = FT_Set_Pixel_Sizes(fontFace, 0, static_cast<FT_UInt>(size));
        if (err) {
            throw std::runtime_error("could not set font size");
        }
    }

    Image FontFinder::renderGlyph(unsigned long glyph, size_t padding, size_t divisibleBy) {
        FT_UInt charIndex = FT_Get_Char_Index(fontFace, static_cast<FT_ULong>(glyph));
        if (charIndex == 0) {
            std::cerr << "Warning: font does not contain glyph with code " << glyph << std::endl;
        }

        FT_Error err = FT_Load_Glyph(fontFace, charIndex, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO);
        FT_Bitmap &bitmap = fontFace->glyph->bitmap;
        if (err || bitmap.buffer == nullptr) {
            throw std::runtime_error("glyph with code " + std::to_string(glyph) + " could not be rendered");
        }
        return {bitmap, padding, divisibleBy};
    }

    std::vector<Image> FontFinder::renderGlyphs(const std::set<unsigned long> &glyphs, int size, size_t padding, size_t divisibleBy) {
        setFontSize(size);

        std::vector<Image> v;
        v.reserve(glyphs.size());
        for (const auto glyph : glyphs) {
            Image img = renderGlyph(glyph, padding, divisibleBy);
            v.push_back(std::move(img));
        }
        return v;
    }
}
