#pragma once

#include <llassetgen/llassetgen_api.h>
#include <llassetgen/packing/Types.h>

namespace llassetgen {
    namespace internal {
        class LLASSETGEN_API ShelfPacker {
           public:
            ShelfPacker(const Vec2<PackingSizeType>& initialAtlasSize, bool _allowRotations, bool _allowGrowth)
                : atlasSize_{initialAtlasSize}, allowRotations{_allowRotations}, allowGrowth{_allowGrowth} {}

            Vec2<PackingSizeType> atlasSize() const { return atlasSize_; }
            bool pack(Rect<PackingSizeType>& rect);

           private:
            LLASSETGEN_NO_EXPORT bool packNoRotations(Rect<PackingSizeType>& rect);
            LLASSETGEN_NO_EXPORT bool packWithRotations(Rect<PackingSizeType>& rect);
            LLASSETGEN_NO_EXPORT bool placeMaybeGrow(Rect<PackingSizeType>& rect);
            LLASSETGEN_NO_EXPORT void place(Rect<PackingSizeType>& rect);
            LLASSETGEN_NO_EXPORT void openNewShelf();

            Vec2<PackingSizeType> atlasSize_;
            bool allowRotations;
            bool allowGrowth;
            Vec2<PackingSizeType> currentShelfSize{0, 0};
            PackingSizeType usedHeight{0};
        };
    }
}
