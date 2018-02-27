#include <llassetgen/Packing.h>

#include <algorithm>
#include <tuple>

namespace llassetgen {
    Packing::Packing(Vec2<PackingSizeType> _atlasSize) : atlasSize{_atlasSize} {}

    namespace internal {
        bool ShelfNextFitPacker::packNext(Vec2<PackingSizeType> rectSize) {
            if (currentShelfSize.x + rectSize.x > packing.atlasSize.x) {
                openNewShelf();
                if (rectSize.x > packing.atlasSize.x) {
                    return false;
                }
            }

            if (usedHeight + rectSize.y > packing.atlasSize.y) {
                return false;
            }

            store(rectSize);
            return true;
        }

        bool ShelfNextFitPacker::packNextRotatable(Vec2<PackingSizeType> rectSize) {
            PackingSizeType minSide, maxSide;
            Vec2<PackingSizeType> rotated;
            std::tie(minSide, maxSide) = std::minmax(rectSize.x, rectSize.y);
            PackingSizeType remainingWidth = packing.atlasSize.x - currentShelfSize.x;
            PackingSizeType remainingHeight = packing.atlasSize.y - usedHeight;

            if (currentShelfSize.y >= maxSide && remainingWidth >= minSide) {
                rotated = {minSide, maxSide};
            } else if (remainingWidth >= maxSide && remainingHeight >= minSide) {
                rotated = {maxSide, minSide};
            } else {
                openNewShelf();
                if (usedHeight + minSide > packing.atlasSize.y) {
                    return false;
                }

                if (maxSide > packing.atlasSize.x) {
                    if (usedHeight + maxSide > packing.atlasSize.x) {
                        return false;
                    }

                    rotated = {minSide, maxSide};
                } else {
                    rotated = {maxSide, minSide};
                }
            }

            store(rotated);
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

        Vec2<PackingSizeType> nextLargerAtlasSize(const Vec2<PackingSizeType>& previous) {
            if (previous.x > previous.y) {
                return {previous.x, 2 * previous.y};
            }

            return {2 * previous.x, previous.y};
        }
    }
}
