
#pragma once


#include <llassetgen/llassetgen_api.h>


struct FT_LibraryRec_;


namespace llassetgen
{


LLASSETGEN_API extern FT_LibraryRec_* freetype;

LLASSETGEN_API void init();


} // namespace llassetgen
