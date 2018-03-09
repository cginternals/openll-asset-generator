#pragma once

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

            bool pack(Rect<PackingSizeType>& rect);

           private:
            LLASSETGEN_NO_EXPORT Rect<PackingSizeType>& findFreeRect(Rect<PackingSizeType>& rect);
            LLASSETGEN_NO_EXPORT void cropRects(const Rect<PackingSizeType>& placedRect);
            LLASSETGEN_NO_EXPORT void pruneFreeList();

            std::vector<Rect<PackingSizeType>> freeList;
        };
    }
}
