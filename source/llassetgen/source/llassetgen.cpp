#pragma once

#include <ft2build.h>
#include <freetype/freetype.h>

#include <llassetgen/llassetgen.h>



namespace llassetgen {
    FT_Library freetype;

    void init() {
        assert(FT_Init_FreeType(&freetype) == 0);
    }
}
