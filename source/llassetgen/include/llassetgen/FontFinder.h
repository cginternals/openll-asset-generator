#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <string>
#ifdef _WIN32
#include <vector>
#endif

#include <llassetgen/llassetgen.h>
#include <llassetgen/llassetgen_api.h>
#include <set>

namespace llassetgen {
    class LLASSETGEN_API FontFinder {
       public:
        static FontFinder fromName(const std::string& fontName);
        static FontFinder fromPath(const std::string& fontPath);

        void setFontSize(int size);
        Image renderGlyph(unsigned long glyph, size_t padding = 0);
        std::vector<Image> renderGlyphs(const std::set<unsigned long>& glyphs, int size, size_t padding = 0);

       private:
        FontFinder() = default;

#ifdef _WIN32
        bool getFontData(const std::string& fontName);
#elif __unix__
        static bool findFontPath(const std::string& fontName, std::string& fontPath);
#endif

        FT_Face fontFace;
#ifdef _WIN32
        std::vector<FT_Byte> fontData;
#endif
    };
}