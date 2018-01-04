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
    using PackingSizeType = uint32_t;

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
    template <class Iter>
    boost::optional<Packing> shelfPackAtlas(Iter sizesBegin, Iter sizesEnd, Vec2<PackingSizeType> atlasSize,
                                            bool allowRotations) {
        impl_packing::assertCorrectIteratorType<Iter>();

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
