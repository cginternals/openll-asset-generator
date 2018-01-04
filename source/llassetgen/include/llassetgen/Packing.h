#pragma once

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <vector>

#include <boost/optional.hpp>

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

        explicit Packing(Vec2<PackingSizeType> _atlasSize);
    };

    namespace impl_packing {
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
        LLASSETGEN_API unsigned int ceilLog2(uint64_t i);

        /**
         * Find the smallest atlas size that might fit the given rectangles.
         *
         * The returned sizes will always be powers of two, and will be as close
         * to square as possible.
         */
        template <class ForwardIter>
        Vec2<PackingSizeType> predictAtlasSize(ForwardIter sizesBegin, ForwardIter sizesEnd) {
            uint64_t areaSum;
            Vec2<PackingSizeType> maxSides;
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

        /**
         * Returns the next bigger atlas size to test for packing.
         */
        LLASSETGEN_API Vec2<PackingSizeType> nextLargerAtlasSize(Vec2<PackingSizeType> previous);

        class LLASSETGEN_API ShelfNextFitPacker {
           public:
            ShelfNextFitPacker(Vec2<PackingSizeType> atlasSize, size_t rectCount);

            bool packNext(Vec2<PackingSizeType> rectSize);

            bool packNextRotatable(Vec2<PackingSizeType> rectSize);

            Packing packing;

           private:
            LLASSETGEN_NO_EXPORT void openNewShelf();

            LLASSETGEN_NO_EXPORT void store(Vec2<PackingSizeType> rectSize);

            Vec2<PackingSizeType> currentShelfSize{0, 0};
            PackingSizeType usedHeight{0};
        };
    }

    /**
     * Use the shelf next fit algorithm to pack a texture atlas.
     *
     * See the fixed size overload for a description of the algorithm.
     *
     * The pair of iterators must be ForwardIterators, i.e. allow iterating the
     * input multiple times. Their items must be `Vec2<PackingSizeType>`s, or
     * convertible to it.
     */
    template <class ForwardIter>
    Packing shelfPackAtlas(ForwardIter sizesBegin, ForwardIter sizesEnd, bool allowRotations) {
        impl_packing::assertCorrectIteratorType<ForwardIter>();

        Vec2<PackingSizeType> atlasSize = impl_packing::predictAtlasSize(sizesBegin, sizesEnd);
        while (true) {
            auto maybePacking = shelfPackAtlas(sizesBegin, sizesEnd, atlasSize, allowRotations);
            if (maybePacking) {
                return *maybePacking;
            }

            atlasSize = impl_packing::nextLargerAtlasSize(atlasSize);
        }
    }

    /**
     * Use the shelf next fit algorithm to pack a fixed size texture atlas.
     *
     * This algorithm can produce suboptimal packings, but has very fast runtime
     * (O(n), where n is the number of rects to pack).
     *
     * Returns an empty optional if the input rectangles can't be fit into the
     * given atlas size.
     *
     * The pair of iterators must be ForwardIterators, i.e. allow iterating the
     * input multiple times. Their items must be `Vec2<PackingSizeType>`s, or
     * convertible to it.
     */
    template <class ForwardIter>
    boost::optional<Packing> shelfPackAtlas(ForwardIter sizesBegin, ForwardIter sizesEnd,
                                            Vec2<PackingSizeType> atlasSize, bool allowRotations) {
        impl_packing::assertCorrectIteratorType<ForwardIter>();

        auto rectCount = static_cast<size_t>(std::distance(sizesBegin, sizesEnd));
        impl_packing::ShelfNextFitPacker packer{atlasSize, rectCount};

        bool allPacked;
        if (allowRotations) {
            allPacked = std::all_of(sizesBegin, sizesEnd,
                                    [&](Vec2<PackingSizeType> rectSize) { return packer.packNextRotatable(rectSize); });
        } else {
            allPacked = std::all_of(sizesBegin, sizesEnd,
                                    [&](Vec2<PackingSizeType> rectSize) { return packer.packNext(rectSize); });
        }

        if (allPacked) {
            return std::move(packer.packing);
        }

        return boost::none;
    }
}
