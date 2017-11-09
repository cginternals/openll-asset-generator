#pragma once

#include <assert.h>
#include <cmath>
#include <memory>
#include <string>

#include "DistanceTransform.h"



struct FT_LibraryRec_;

namespace llassetgen {
    extern FT_LibraryRec_* freetype;

    void init();
};
