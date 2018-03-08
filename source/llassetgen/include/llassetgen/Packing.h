#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <vector>

#include <llassetgen/Geometry.h>
#include <llassetgen/llassetgen_api.h>

namespace llassetgen {
    using PackingSizeType = unsigned int;

    /*
     * Describes the packing of a texture atlas.
     *
     * The rectangles in `rects` correspond to the input rectangles at the same
     * position in the packing algorithms input.
     */
    struct Packing {
        Vec2<PackingSizeType> atlasSize{};
        std::vector<Rect<PackingSizeType>> rects{};

        Packing() = default;

        LLASSETGEN_API explicit Packing(Vec2<PackingSizeType> _atlasSize);
    };

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

    /**
     * Use the shelf next fit algorithm to pack a texture atlas.
     *
     * See the fixed size overload for a description of the algorithm.
     *
     * @param sizesBegin
     *   Begin iterator for the sizes of the input rectangles. This iterators
     *   items must be convertible to `Vec2<PackingSizeType>`. It must also be
     *   a ForwardIterator, i.e. allow iterating over the input multiple times.
     * @param sizesEnd
     *   End iterator for the rectangle sizes. Same constraints as for
     *   `sizesBegin` apply.
     * @param allowRotations
     *   Whether to allow rotating rectangles by 90˚.
     * @return
     *   Resulting packing.
     */
    template <class ForwardIter>
    Packing shelfPackAtlas(ForwardIter sizesBegin, ForwardIter sizesEnd, bool allowRotations) {
        internal::assertCorrectIteratorType<ForwardIter>();

        Packing packing{internal::predictAtlasSize(sizesBegin, sizesEnd)};
        bool success = internal::shelfPackAtlas(sizesBegin, sizesEnd, packing, allowRotations, true);
        // Only fails when a rectangle is wider (and higher, if rotations are enabled)
        // than the width of the atlas texture, however this should be prevented
        // by predictAtlasSize.
        assert(success);

        return packing;
    }

    /**
     * Use the shelf next fit algorithm to pack a fixed size texture atlas.
     *
     * This algorithm can produce suboptimal packings, but has very fast runtime
     * (O(n), where n is the number of rectangles to pack).
     *
     * @param sizesBegin
     *   Begin iterator for the sizes of the input rectangles. This iterators
     *   items must be convertible to `Vec2<PackingSizeType>`. It must also be
     *   a ForwardIterator, i.e. allow iterating over the input multiple times.
     * @param sizesEnd
     *   End iterator for the rectangle sizes. Same constraints as for
     *   `sizesBegin` apply.
     * @param atlasSize
     *   Size of the atlas to pack into.
     * @param allowRotations
     *   Whether to allow rotating rectangles by 90˚.
     * @return
     *   Resulting packing. If the given rectangles can't be fit into the atlas
     *   size, the list of rectangles will be empty.
     */
    template <class ForwardIter>
    Packing shelfPackAtlas(ForwardIter sizesBegin, ForwardIter sizesEnd, Vec2<PackingSizeType> atlasSize,
                           bool allowRotations) {
        internal::assertCorrectIteratorType<ForwardIter>();

        Packing packing{atlasSize};
        bool success = internal::shelfPackAtlas(sizesBegin, sizesEnd, packing, allowRotations, false);
        if (!success) {
            packing.rects.clear();
        }

        return packing;
    }
}
