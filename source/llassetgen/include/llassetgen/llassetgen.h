#pragma once

#include <cassert>
#include <cmath>
#include <memory>
#include <string>

#include "DistanceTransform.h"
#include "Packing.h"

#include <llassetgen/llassetgen_api.h>

struct FT_LibraryRec_;

namespace llassetgen {
    LLASSETGEN_API extern FT_LibraryRec_* freetype;

    LLASSETGEN_API void init();
}
