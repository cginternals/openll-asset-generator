#pragma once

#include <cassert>

#include <llassetgen/packing/Types.h>
#include <llassetgen/packing/internal/ShelfPacker.h>

namespace llassetgen {
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
