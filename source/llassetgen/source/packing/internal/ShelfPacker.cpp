#include <llassetgen/packing/internal/ShelfPacker.h>

#include <tuple>
#include <utility>

#include <llassetgen/packing/internal/Common.h>


namespace
{


using llassetgen::PackingSizeType;

PackingSizeType ceilDiv(const PackingSizeType dividend, const PackingSizeType divisor)
{
    return (dividend + divisor - PackingSizeType(1)) / divisor;
}

template <class T>
std::pair<T, T> flip(const std::pair<T, T> & pair)
{
    return {pair.second, pair.first};
}


} // namespace


namespace llassetgen
{
namespace internal
{


bool ShelfPacker::inputSortingComparator(const Rect<PackingSizeType>& rect1,
                                         const Rect<PackingSizeType>& rect2)
{
    // Sort by longest side descending (DESCLS)
    return flip(std::minmax(rect1.size.x, rect1.size.y)) > flip(std::minmax(rect2.size.x, rect2.size.y));
}

bool ShelfPacker::pack(Rect<PackingSizeType>& rect)
{
    return allowRotations ? packWithRotations(rect) : packNoRotations(rect);
}

bool ShelfPacker::packNoRotations(Rect<PackingSizeType>& rect)
{
    if (currentShelfSize.x + rect.size.x > atlasSize_.x)
    {
        openNewShelf();
        if (rect.size.x > atlasSize_.x)
        {
            return false;
        }
    }

    return placeMaybeGrow(rect);
}

bool ShelfPacker::packWithRotations(Rect<PackingSizeType>& rect)
{
    PackingSizeType minSide, maxSide;
    std::tie(minSide, maxSide) = std::minmax(rect.size.x, rect.size.y);
    PackingSizeType remainingWidth = atlasSize_.x - currentShelfSize.x;
    PackingSizeType remainingHeight = atlasSize_.y - usedHeight;

    if (currentShelfSize.y >= maxSide && remainingWidth >= minSide)
    {
        rect.size = {minSide, maxSide};
        place(rect);
    }
    else if (remainingWidth >= maxSide && remainingHeight >= minSide)
    {
        rect.size = {maxSide, minSide};
        place(rect);
    }
    else
    {
        openNewShelf();
        rect.size = maxSide > atlasSize_.x ? Vec2<PackingSizeType>{minSide, maxSide} : Vec2<PackingSizeType>{maxSide, minSide};

        return placeMaybeGrow(rect);
    }

    return true;
}

void ShelfPacker::openNewShelf()
{
    usedHeight += currentShelfSize.y;
    currentShelfSize = {0, 0};
}

bool ShelfPacker::placeMaybeGrow(Rect<PackingSizeType>& rect)
{
    if (usedHeight + rect.size.y > atlasSize_.y)
    {
        if (!allowGrowth)
        {
            return false;
        }

        PackingSizeType finalHeight = usedHeight + rect.size.y;
        auto numDoublings = ceilLog2(ceilDiv(finalHeight, atlasSize_.y));
        atlasSize_.y <<= numDoublings;
    }

    place(rect);

    return true;
}

void ShelfPacker::place(Rect<PackingSizeType>& rect)
{
    rect.position = {currentShelfSize.x, usedHeight};
    currentShelfSize.x += rect.size.x;
    currentShelfSize.y = std::max(currentShelfSize.y, rect.size.y);
}


} // namespace internal
} // namespace llassetgen
