#pragma once

#include <cassert>

#include "DistanceTransform.h"
#include "FntWriter.h"

struct FT_LibraryRec_;

namespace llassetgen {
    LLASSETGEN_API extern FT_LibraryRec_* freetype;

    LLASSETGEN_API void init();
};
