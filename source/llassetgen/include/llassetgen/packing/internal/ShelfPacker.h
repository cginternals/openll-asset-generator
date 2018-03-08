#pragma once

#include <algorithm>
#include <iterator>
#include <type_traits>

#include <llassetgen/llassetgen_api.h>
#include <llassetgen/packing/Types.h>

namespace llassetgen {
    namespace internal {
        template <class Iter>
        constexpr int assertCorrectIteratorType() {
            using IterTraits = typename std::iterator_traits<Iter>;
            using IterRefType = typename IterTraits::reference;
            using IterCategory = typename IterTraits::iterator_category;
            static_assert(std::is_convertible<IterRefType, const Vec2<PackingSizeType>&>::value,
                          "Packing input must be convertible to const Vec2<PackingSizeType>&");
            static_assert(std::is_base_of<std::forward_iterator_tag, IterCategory>::value,
                          "Iterator must be a ForwardIterator");

            // C++11 doesn't allow void as the return type of constexpr functions
            return 0;
        }

        /**
         * Return the smallest integer x so that i <= 2^x.
         */
        LLASSETGEN_API unsigned int ceilLog2(uint64_t num);

        /**
         * Find the smallest atlas size that might fit the given rectangles.
         *
         * The returned sizes will always be powers of two, and will be as close
         * to square as possible.
         */
        template <class ForwardIter>
        Vec2<PackingSizeType> predictAtlasSize(ForwardIter sizesBegin, ForwardIter sizesEnd) {
            uint64_t areaSum{0};
            Vec2<PackingSizeType> maxSides{0, 0};
            std::for_each(sizesBegin, sizesEnd, [&](Vec2<PackingSizeType> rectSize) {
                areaSum += static_cast<uint64_t>(rectSize.x) * rectSize.y;
                maxSides.x = std::max(maxSides.x, rectSize.x);
                maxSides.y = std::max(maxSides.y, rectSize.y);
            });

            auto areaExponent = ceilLog2(areaSum);
            auto heightExponent = areaExponent / 2;
            auto widthExponent = areaExponent - heightExponent;
            auto minWidthExponent = ceilLog2(maxSides.x);
            auto minHeightExponent = ceilLog2(maxSides.y);

            if (widthExponent < minWidthExponent) {
                widthExponent = minWidthExponent;
                heightExponent = std::max(minHeightExponent, areaExponent - widthExponent);
            } else if (heightExponent < minHeightExponent) {
                heightExponent = minHeightExponent;
                widthExponent = std::max(minWidthExponent, areaExponent - heightExponent);
            }

            return {1u << widthExponent, 1u << heightExponent};
        }

        class LLASSETGEN_API ShelfNextFitPacker {
           public:
            explicit ShelfNextFitPacker(Packing& _packing, bool _allowRotations, bool _allowGrowth)
                : packing(_packing), allowRotations{_allowRotations}, allowGrowth{_allowGrowth} {};

            bool packNext(Vec2<PackingSizeType> rectSize);

           private:
            LLASSETGEN_NO_EXPORT bool packNextNoRotations(Vec2<PackingSizeType> rectSize);
            LLASSETGEN_NO_EXPORT bool packNextWithRotations(Vec2<PackingSizeType> rectSize);
            LLASSETGEN_NO_EXPORT void openNewShelf();
            LLASSETGEN_NO_EXPORT bool storeMaybeGrow(Vec2<PackingSizeType> rectSize);
            LLASSETGEN_NO_EXPORT void store(Vec2<PackingSizeType> rectSize);

            Packing& packing;
            bool allowRotations;
            bool allowGrowth;
            Vec2<PackingSizeType> currentShelfSize{0, 0};
            PackingSizeType usedHeight{0};
        };

        template <class ForwardIter>
        bool shelfPackAtlas(ForwardIter sizesBegin, ForwardIter sizesEnd, Packing& packing, bool allowRotations,
                            bool allowGrowth) {
            auto rectCount = static_cast<size_t>(std::distance(sizesBegin, sizesEnd));
            packing.rects.reserve(rectCount);

            ShelfNextFitPacker packer{packing, allowRotations, allowGrowth};
            return std::all_of(sizesBegin, sizesEnd,
                               [&](Vec2<PackingSizeType> rectSize) { return packer.packNext(rectSize); });
        }
    }
}
