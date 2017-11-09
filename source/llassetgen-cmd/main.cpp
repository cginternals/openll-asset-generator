#include <iostream>

#include <ft2build.h>
#include <freetype/freetype.h>

#include <llassetgen/llassetgen.h>



using namespace llassetgen;

int main(int argc, char** argv) {
    llassetgen::init();

    if(argc == 5 && strcmp(argv[1], "-dt-from-glyph") == 0) {
        // Example: llassetgen-cmd -dt-from-glyph G /Library/Fonts/Verdana.ttf glyph.png
        FT_Face face;
        assert(FT_New_Face(freetype, argv[3], 0, &face) == 0);
        assert(FT_Set_Pixel_Sizes(face, 0, 32) == 0);
        assert(FT_Load_Glyph(face, FT_Get_Char_Index(face, argv[2][0]), FT_LOAD_RENDER | FT_LOAD_TARGET_MONO) == 0);
        DistanceTransform dt;
        dt.importFreeTypeBitmap(&face->glyph->bitmap, 8);
        dt.transform();
        dt.exportPng(argv[4], -5, 8, 8);
    } else if(argc == 4 && strcmp(argv[1], "-dt-from-png") == 0) {
        // Example: llassetgen-cmd -dt-from-png input.png output.png
        DistanceTransform dt;
        dt.importPng(argv[2]);
        dt.transform();
        dt.exportPng(argv[3], -20, 50, 8);
    }

    return 0;
}
