#pragma once

#include <ft2build.h>  // NOLINT include order required by freetype
#include FT_FREETYPE_H
#include <llassetgen/llassetgen.h>

#include <string>

using namespace llassetgen;

class FontRenderer {
   public:
    static FontRenderer fromName(const std::string& fontName);
    static FontRenderer fromPath(const std::string& fontPath);
    std::unique_ptr<Image> renderGlyph(unsigned long glyph, unsigned int size);

   private:
    FontRenderer() = default;
    FT_Face fontFace;

#ifdef _WIN32
    std::vector<FT_Byte> fontData;
    bool getFontData(const std::string& fontName);
#elif __unix__
    static bool findFontPath(const std::string& fontName, std::string& fontPath);
#endif
};
