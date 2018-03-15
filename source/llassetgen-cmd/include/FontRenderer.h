#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <string>
#ifdef _WIN32
    #include <vector>
#endif

#include <llassetgen/llassetgen.h>

using namespace llassetgen;

class FontRenderer {
   public:
    static FontRenderer fromName(const std::string& fontName);
    static FontRenderer fromPath(const std::string& fontPath);

    std::unique_ptr<Image> renderGlyph(unsigned long glyph, unsigned int size);

   private:
    FontRenderer() = default;

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
