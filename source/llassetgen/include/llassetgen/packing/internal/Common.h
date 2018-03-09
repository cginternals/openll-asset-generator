#pragma once

#include <algorithm>
#include <cassert>
#include <iterator>
#include <type_traits>

#include <llassetgen/llassetgen_api.h>
#include <llassetgen/packing/Types.h>

namespace llassetgen {
    namespace internal {
        template <class InputIter>
        size_t rectReserveCountInternal(InputIter /*unused*/, InputIter /*unused*/,
                                        std::input_iterator_tag /*unused*/) {
            return 0;
        }

        template <class InputIter>
        size_t rectReserveCountInternal(InputIter begin, InputIter end, std::random_access_iterator_tag /*unused*/) {
            return end - begin;
        }

        /**
         * Number of rectangles to reserve for packing.
         *
         * Only returns non-zero values for random access iterators, for all other
         * iterators the only way to determine the number of items is actually
         * iterating them.
         */
        template <class InputIter>
        size_t rectReserveCount(InputIter begin, InputIter end) {
            using IterCategory = typename std::iterator_traits<InputIter>::iterator_category;
            return rectReserveCountInternal(begin, end, IterCategory{});
        }

        template <class Iter>
        constexpr int checkIteratorType() {
            using IterTraits = typename std::iterator_traits<Iter>;
            using IterRefType = typename IterTraits::reference;
            using IterCategory = typename IterTraits::iterator_category;
            static_assert(std::is_assignable<Vec2<PackingSizeType>, IterRefType>::value,
                          "Packing input elements must be assignable to Vec2<PackingSizeType>");
            static_assert(std::is_base_of<std::input_iterator_tag, IterCategory>::value,
                          "Packing input iterator must be an InputIterator");

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
        LLASSETGEN_API Vec2<PackingSizeType> predictAtlasSize(const Packing& packing);

        /**
         * Given rect sizes, create a packing with rectangles with those sizes.
         */
        template <class InputIter>
        Packing initPacking(InputIter sizesBegin, InputIter sizesEnd) {
            checkIteratorType<InputIter>();

            Packing packing;
            packing.rects.reserve(rectReserveCount(sizesBegin, sizesEnd));
            std::transform(sizesBegin, sizesEnd, std::back_inserter(packing.rects),
                           [](const Vec2<PackingSizeType>& rectSize) {
                               return Rect<PackingSizeType>{{0, 0}, rectSize};
                           });
            return packing;
        }

        template <class Packer>
        bool packAll(Packing& packing, Packer& packer) {
            return std::all_of(packing.rects.begin(), packing.rects.end(),
                               [&](Rect<PackingSizeType>& rect) { return packer.pack(rect); });
        }

        /**
         * Create a flexible size packing from given rectangle sizes.
         *
         * @tparam Packer
         *   Packer class. Must provide the following methods:
         *    - `Packer(const Vec2<PackingSizeType>& initialAtlasSize, bool allowRotations, bool allowGrowth)`
         *    - `bool pack(Rect<PackingSizeType>& rect)`: Packs the rectangle at
         *      a position, the size is pre-filled. Returns false if the space
         *      is insufficient.
         *    - `Vec2<PackingSizeType> atlasSize() const`: Used to retrieve the
         *      final atlas size after packing (not used for fixed size packing).
         */
        template <class Packer, class InputIter>
        Packing packAtlas(InputIter sizesBegin, InputIter sizesEnd, bool allowRotations) {
            Packing packing = initPacking(sizesBegin, sizesEnd);
            packing.atlasSize = predictAtlasSize(packing);

            Packer packer{packing.atlasSize, allowRotations, true};
            bool success = packAll(packing, packer);
            // Only fails when a rectangle is wider (and higher, if rotations are enabled)
            // than the width of the atlas texture, however this should be prevented
            // by predictAtlasSize.
            assert(success);

            packing.atlasSize = packer.atlasSize();
            return packing;
        }

        /**
         * Create a fixed size packing from given rectangle sizes.
         *
         * @tparam Packer
         *   Refer to flexible size overload.
         */
        template <class Packer, class InputIter>
        Packing packAtlas(InputIter sizesBegin, InputIter sizesEnd, bool allowRotations,
                          const Vec2<PackingSizeType>& fixedAtlasSize) {
            Packing packing = initPacking(sizesBegin, sizesEnd);
            packing.atlasSize = fixedAtlasSize;

            Packer packer{packing.atlasSize, allowRotations, false};
            bool success = packAll(packing, packer);
            if (!success) {
                packing.rects.clear();
            }

            return packing;
        }
    }
}
