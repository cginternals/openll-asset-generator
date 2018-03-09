#include <llassetgen/packing/internal/ShelfPacker.h>

#include <tuple>

#include <llassetgen/packing/internal/Common.h>

using llassetgen::PackingSizeType;

PackingSizeType ceilDiv(PackingSizeType dividend, PackingSizeType divisor) {
    return (dividend + divisor - 1) / divisor;
}

namespace llassetgen {
    namespace internal {
        bool ShelfPacker::pack(Rect<PackingSizeType>& rect) {
            return allowRotations ? packWithRotations(rect) : packNoRotations(rect);
        }

        bool ShelfPacker::packNoRotations(Rect<PackingSizeType>& rect) {
            if (currentShelfSize.x + rect.size.x > atlasSize_.x) {
                openNewShelf();
                if (rect.size.x > atlasSize_.x) {
                    return false;
                }
            }

            return placeMaybeGrow(rect);
        }

        bool ShelfPacker::packWithRotations(Rect<PackingSizeType>& rect) {
            PackingSizeType minSide, maxSide;
            std::tie(minSide, maxSide) = std::minmax(rect.size.x, rect.size.y);
            PackingSizeType remainingWidth = atlasSize_.x - currentShelfSize.x;
            PackingSizeType remainingHeight = atlasSize_.y - usedHeight;

            if (currentShelfSize.y >= maxSide && remainingWidth >= minSide) {
                rect.size = {minSide, maxSide};
                place(rect);
            } else if (remainingWidth >= maxSide && remainingHeight >= minSide) {
                rect.size = {maxSide, minSide};
                place(rect);
            } else {
                openNewShelf();
                if (maxSide > atlasSize_.x) {
                    rect.size = {minSide, maxSide};
                } else {
                    rect.size = {maxSide, minSide};
                }

                return placeMaybeGrow(rect);
            }

            return true;
        }

        void ShelfPacker::openNewShelf() {
            usedHeight += currentShelfSize.y;
            currentShelfSize = {0, 0};
        }

        bool ShelfPacker::placeMaybeGrow(Rect<PackingSizeType>& rect) {
            if (usedHeight + rect.size.y > atlasSize_.y) {
                if (allowGrowth) {
                    PackingSizeType finalHeight = usedHeight + rect.size.y;
                    auto numDoublings = ceilLog2(ceilDiv(finalHeight, atlasSize_.y));
                    atlasSize_.y <<= numDoublings;
                } else {
                    return false;
                }
            }

            place(rect);
            return true;
        }

        void ShelfPacker::place(Rect<PackingSizeType>& rect) {
            rect.position = {currentShelfSize.x, usedHeight};
            currentShelfSize.x += rect.size.x;
            currentShelfSize.y = std::max(currentShelfSize.y, rect.size.y);
        }
    }
}
