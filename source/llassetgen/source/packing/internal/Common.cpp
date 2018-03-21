#include <llassetgen/packing/internal/Common.h>

namespace llassetgen {
    namespace internal {
        unsigned int ceilLog2(uint64_t num) {
            // Do binary search for the highest set bit in `num` by testing
            // whether an increasingly narrow range of bits is zero.
            //
            // Example for the first step: Are the highest 32 bits zero?
            //   - yes -> continue search in the lower 32 bits
            //   - no  -> result at least 32, continue search in higher 32 bits

            static constexpr std::uint64_t rangeMasks[6] = {0xFFFFFFFF00000000ull, 0x00000000FFFF0000ull,
                                                            0x000000000000FF00ull, 0x00000000000000F0ull,
                                                            0x000000000000000Cull, 0x0000000000000002ull};

            // Start with 1 if the input is a power of two
            auto result = static_cast<unsigned int>((num & (num - 1)) != 0);
            unsigned int rangeWidth = 32;
            for (std::uint64_t rangeMask : rangeMasks) {
                auto rangeNotEmpty = static_cast<unsigned int>((num & rangeMask) != 0);
                result += rangeNotEmpty * rangeWidth;
                num >>= rangeNotEmpty * rangeWidth;
                rangeWidth /= 2;
            }

            return result;
        }

        Vec2<PackingSizeType> predictAtlasSize(const Packing& packing) {
            uint64_t areaSum{0};
            Vec2<PackingSizeType> maxSides{0, 0};
            std::for_each(packing.rects.begin(), packing.rects.end(), [&](const Rect<PackingSizeType>& rect) {
                areaSum += static_cast<uint64_t>(rect.size.x) * rect.size.y;
                maxSides.x = std::max(maxSides.x, rect.size.x);
                maxSides.y = std::max(maxSides.y, rect.size.y);
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
    }
}
