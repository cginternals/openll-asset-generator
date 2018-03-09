#pragma once

#include <cassert>

#include <llassetgen/packing/Types.h>
#include <llassetgen/packing/internal/Common.h>
#include <llassetgen/packing/internal/ShelfPacker.h>

namespace llassetgen {
    /**
     * Use the shelf next fit algorithm to pack a texture atlas.
     *
     * See the fixed size overload for a description of the algorithm.
     *
     * @param sizesBegin
     *   Begin iterator for the sizes of the input rectangles. This iterators
     *   items must be convertible to `Vec2<PackingSizeType>`.
     * @param sizesEnd
     *   End iterator for the rectangle sizes.
     * @param allowRotations
     *   Whether to allow rotating rectangles by 90˚.
     * @return
     *   Resulting packing.
     */
    template <class InputIter>
    Packing shelfPackAtlas(InputIter sizesBegin, InputIter sizesEnd, bool allowRotations) {
        return internal::packAtlas<internal::ShelfPacker>(sizesBegin, sizesEnd, allowRotations);
    }

    /**
     * Use the shelf next fit algorithm to pack a fixed size texture atlas.
     *
     * This algorithm can produce suboptimal packings, but has very fast runtime
     * (O(n), where n is the number of rectangles to pack).
     *
     * @param sizesBegin
     *   Begin iterator for the sizes of the input rectangles. This iterators
     *   items must be convertible to `Vec2<PackingSizeType>`.
     * @param sizesEnd
     *   End iterator for the rectangle sizes.
     * @param fixedAtlasSize
     *   Size of the atlas to pack into.
     * @param allowRotations
     *   Whether to allow rotating rectangles by 90˚.
     * @return
     *   Resulting packing. If the given rectangles can't be fit into the atlas
     *   size, the list of rectangles will be empty.
     */
    template <class InputIter>
    Packing shelfPackAtlas(InputIter sizesBegin, InputIter sizesEnd, Vec2<PackingSizeType> fixedAtlasSize,
                           bool allowRotations) {
        return internal::packAtlas<internal::ShelfPacker>(sizesBegin, sizesEnd, allowRotations, fixedAtlasSize);
    }
}
