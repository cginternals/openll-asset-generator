#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <string>
#ifdef _WIN32
    #include <vector>
#endif

#include <llassetgen/llassetgen.h>

namespace llassetgen {
    class FontFinder {
       public:
        static FontFinder fromName(const std::string &fontName);
        static FontFinder fromPath(const std::string &fontPath);

        Image renderGlyph(unsigned long glyph, int size, size_t padding = 0);
        void renderGlyphs(const std::u32string &glyphs, std::vector<Image> &v, int size, size_t padding = 0);

       private:
        FontFinder() = default;

#ifdef _WIN32
        bool getFontData(const std::string& fontName);
#elif __unix__
        static bool findFontPath(const std::string &fontName, std::string &fontPath);
#endif

        FT_Face fontFace;
#ifdef _WIN32
        std::vector<FT_Byte> fontData;
#endif
    };
}