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

        std::vector<Image> renderGlyphs(const std::set<unsigned long>& glyphs, int size, size_t padding = 0,
                                        size_t divisibleBy = 1);
        std::set<FT_ULong> nonDepictableChars;
        FT_Face fontFace;

       private:
        FontFinder() = default;

#ifdef _WIN32
        bool getFontData(const std::string& fontName);
#elif defined(__unix__) || defined(__APPLE__)
        static bool findFontPath(const std::string& fontName, std::string& fontPath);
#endif

#ifdef _WIN32
        std::vector<FT_Byte> fontData;
#endif
    };
}
