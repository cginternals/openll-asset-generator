#pragma once

#include <algorithm>
#include <vector>

#include <llassetgen/llassetgen_api.h>
#include <llassetgen/packing/Types.h>
#include <llassetgen/packing/internal/Common.h>

namespace llassetgen {
    namespace internal {
        class LLASSETGEN_API MaxRectsPacker : public BasePacker {
           public:
            MaxRectsPacker(const Vec2<PackingSizeType>& initialAtlasSize, bool _allowRotations, bool _allowGrowth)
                : BasePacker{initialAtlasSize, _allowRotations, _allowGrowth}, freeList{{{0, 0}, initialAtlasSize}} {}

            static bool inputSortingComparator(const Rect<PackingSizeType>& rect1, const Rect<PackingSizeType>& rect2);

            bool pack(Rect<PackingSizeType>& rect);

           private:
            LLASSETGEN_NO_EXPORT std::vector<Rect<PackingSizeType>>::iterator findFreeRect(Rect<PackingSizeType>& rect);
            LLASSETGEN_NO_EXPORT void grow();
            LLASSETGEN_NO_EXPORT void cropRects(const Rect<PackingSizeType>& placedRect);
            LLASSETGEN_NO_EXPORT void pruneFreeList();

            std::vector<Rect<PackingSizeType>> freeList;
        };
    }
}
