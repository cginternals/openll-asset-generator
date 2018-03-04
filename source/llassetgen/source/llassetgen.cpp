#include <ft2build.h> // NOLINT include order required by freetype
#include FT_FREETYPE_H

#include <llassetgen/llassetgen.h>

namespace llassetgen {
    FT_Library freetype;

    void init() { assert(FT_Init_FreeType(&freetype) == 0); }
}
