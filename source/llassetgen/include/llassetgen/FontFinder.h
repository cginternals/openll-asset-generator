
#pragma once

#include <string>
#ifdef _WIN32
#include <vector>
#endif

#include <set>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <llassetgen/llassetgen_api.h>
#include <llassetgen/Image.h>


namespace llassetgen
{


class LLASSETGEN_API FontFinder
{
public:
    static FontFinder fromName(const std::string& fontName);
    static FontFinder fromPath(const std::string& fontPath);

    void setFontSize(int size);
    Image renderGlyph(unsigned long glyph, size_t padding, size_t divisibleBy);

    std::vector<Image> renderGlyphs(const std::set<unsigned long>& glyphs, int size, size_t padding = 0,
                                    size_t divisibleBy = 1);

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

} // namespace llassetgen
