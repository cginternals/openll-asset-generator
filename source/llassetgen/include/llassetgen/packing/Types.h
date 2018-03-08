#pragma once

#include <vector>

#include <llassetgen/Geometry.h>

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

        explicit Packing(Vec2<PackingSizeType> _atlasSize) : atlasSize{_atlasSize} {};
    };
}
