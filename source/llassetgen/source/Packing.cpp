#include <llassetgen/Packing.h>

#include <algorithm>
#include <tuple>

using llassetgen::PackingSizeType;

PackingSizeType ceilDiv(PackingSizeType dividend, PackingSizeType divisor) {
    return (dividend + divisor - 1) / divisor;
}

namespace llassetgen {
    Packing::Packing(Vec2<PackingSizeType> _atlasSize) : atlasSize{_atlasSize} {}

    namespace internal {
        bool ShelfNextFitPacker::packNext(Vec2<PackingSizeType> rectSize) {
            return allowRotations ? packNextWithRotations(rectSize) : packNextNoRotations(rectSize);
        }

        bool ShelfNextFitPacker::packNextNoRotations(Vec2<PackingSizeType> rectSize) {
            if (currentShelfSize.x + rectSize.x > packing.atlasSize.x) {
                openNewShelf();
                if (rectSize.x > packing.atlasSize.x) {
                    return false;
                }
            }

            if (usedHeight + rectSize.y > packing.atlasSize.y) {
                if (allowGrowth) {
                    PackingSizeType finalHeight = usedHeight + rectSize.y;
                    auto numDoublings = ceilLog2(ceilDiv(finalHeight, packing.atlasSize.y));
                    packing.atlasSize.y <<= numDoublings;
                } else {
                    return false;
                }
            }

            store(rectSize);
            return true;
        }

        bool ShelfNextFitPacker::packNextWithRotations(Vec2<PackingSizeType> rectSize) {
            PackingSizeType minSide, maxSide;
            std::tie(minSide, maxSide) = std::minmax(rectSize.x, rectSize.y);
            PackingSizeType remainingWidth = packing.atlasSize.x - currentShelfSize.x;
            PackingSizeType remainingHeight = packing.atlasSize.y - usedHeight;

            if (currentShelfSize.y >= maxSide && remainingWidth >= minSide) {
                store({minSide, maxSide});
            } else if (remainingWidth >= maxSide && remainingHeight >= minSide) {
                store({maxSide, minSide});
            } else {
                openNewShelf();
                return maxSide > packing.atlasSize.x ? packNextNoRotations({minSide, maxSide})
                                                     : packNextNoRotations({maxSide, minSide});
            }

            return true;
        }

        void ShelfNextFitPacker::openNewShelf() {
            usedHeight += currentShelfSize.y;
            currentShelfSize = {0, 0};
        }

        void ShelfNextFitPacker::store(Vec2<PackingSizeType> rectSize) {
            packing.rects.push_back({{currentShelfSize.x, usedHeight}, rectSize});
            currentShelfSize.x += rectSize.x;
            currentShelfSize.y = std::max(currentShelfSize.y, rectSize.y);
        }

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
    }
}
