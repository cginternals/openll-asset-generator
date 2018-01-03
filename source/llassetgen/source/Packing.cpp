#include <llassetgen/Packing.h>

#include <algorithm>
#include <tuple>

namespace llassetgen {
    Packing::Packing(Vec2<PackingSizeType> _atlasSize) : atlasSize{_atlasSize} {}

    namespace detail {
        ShelfNextFitPacker::ShelfNextFitPacker(Vec2<PackingSizeType> atlasSize, size_t rectCount) : packing{atlasSize} {
            packing.rects.reserve(rectCount);
        }

        bool ShelfNextFitPacker::packNext(Vec2<PackingSizeType> rectSize) {
            if (currentShelfSize.x + rectSize.x > packing.atlasSize.x) {
                usedHeight += currentShelfSize.y;
                if (rectSize.x > packing.atlasSize.x) {
                    return false;
                }

                currentShelfSize = {0, 0};
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
    }
}
