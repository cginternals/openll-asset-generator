
#pragma once


#include <llassetgen/llassetgen_api.h>
#include <llassetgen/packing/Types.h>
#include <llassetgen/packing/internal/Common.h>


namespace llassetgen
{
namespace internal
{


class LLASSETGEN_API ShelfPacker : public BasePacker
{
public:
    ShelfPacker(const Vec2<PackingSizeType>& initialAtlasSize, bool _allowRotations, bool _allowGrowth)
    : BasePacker{initialAtlasSize, _allowRotations, _allowGrowth}
    {
    }

    static bool inputSortingComparator(const Rect<PackingSizeType>& rect1, const Rect<PackingSizeType>& rect2);

    bool pack(Rect<PackingSizeType>& rect);

private:
    LLASSETGEN_NO_EXPORT bool packNoRotations(Rect<PackingSizeType>& rect);
    LLASSETGEN_NO_EXPORT bool packWithRotations(Rect<PackingSizeType>& rect);
    LLASSETGEN_NO_EXPORT bool placeMaybeGrow(Rect<PackingSizeType>& rect);
    LLASSETGEN_NO_EXPORT void place(Rect<PackingSizeType>& rect);
    LLASSETGEN_NO_EXPORT void openNewShelf();

    Vec2<PackingSizeType> currentShelfSize{0, 0};
    PackingSizeType usedHeight{0};
};


} // namespace internal
} // namespace llassetgen
