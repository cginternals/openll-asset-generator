#pragma once

#include <cstdint>
#include <iterator>
#include <type_traits>
#include <vector>

#include <boost/optional.hpp>

#include <llassetgen/Geometry.h>

namespace llassetgen {
    using PackingSizeType = uint32_t;

    namespace detail {
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
    }

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
    boost::optional<Packing> shelfPackAtlas(Iter sizes_begin, Iter sizes_end, Vec2<PackingSizeType> atlasSize) {
        detail::assertCorrectIteratorType<Iter>();

        auto rectCount = static_cast<size_t>(std::distance(sizes_begin, sizes_end));
        Packing packing{atlasSize};
        packing.rects.reserve(rectCount);

        Vec2<PackingSizeType> currentShelfSize{0, 0};
        PackingSizeType usedHeight{0};
        while (sizes_begin != sizes_end) {
            const Vec2<PackingSizeType>& rectSize = *sizes_begin++;

            if (currentShelfSize.x + rectSize.x > atlasSize.x) {
                usedHeight += currentShelfSize.y;
                if (rectSize.x > atlasSize.x) {
                    return {};
                }

                currentShelfSize = {0, 0};
            }

            if (usedHeight + rectSize.y > atlasSize.y) {
                return {};
            }

            packing.rects.push_back({{currentShelfSize.x, usedHeight}, rectSize});
            currentShelfSize.x += rectSize.x;
            currentShelfSize.y = std::max(currentShelfSize.y, rectSize.y);
        }

        return packing;
    }
}
