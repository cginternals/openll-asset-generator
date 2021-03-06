
#pragma once


#include <llassetgen/DistanceTransform.h>
#include <llassetgen/Image.h>
#include <llassetgen/packing/Types.h>


namespace llassetgen
{


using ImageTransform = void (*)(Image&, Image&);


namespace internal
{


template <class Iter>
constexpr int checkImageIteratorType()
{
    using IterTraits = typename std::iterator_traits<Iter>;
    using IterType = typename IterTraits::value_type;
    using IterCategory = typename IterTraits::iterator_category;
    static_assert(std::is_assignable<Image, IterType>::value, "Input elements must be assignable to Image");
    static_assert(std::is_base_of<std::input_iterator_tag, IterCategory>::value,
                  "Input iterator must be an InputIterator");
    return 0;
}


} // namespace


template <class ImageIter>
Image fontAtlas(const ImageIter imgBegin, const ImageIter imgEnd, const Packing & packing, const uint8_t bitDepth = 1)
{
    internal::checkImageIteratorType<ImageIter>();
    assert(std::distance(imgBegin, imgEnd) == static_cast<typename std::iterator_traits<ImageIter>::difference_type>(packing.rects.size()));

    Image atlas{packing.atlasSize.x, packing.atlasSize.y, bitDepth};
    atlas.clear();

    auto rectIt = packing.rects.begin();
    for (auto it = imgBegin; it < imgEnd; ++rectIt, ++it)
    {
        Image view = atlas.view(rectIt->position, rectIt->position + rectIt->size);
        view.copyDataFrom(*it);
    }
    return atlas;
}

/*
 * Every Image corresponds to a Rect from the Packing. If a Rect's size is smaller than its Image's
 * size, the Image will be downsampled in the returned atlas. The downsampling ratio is determined by
 * dividing the Image's size by its Rect's size. Only integer ratios are allowed: if the division
 * has a remainder, an error will occur.
 */
template <class ImageIter>
Image distanceFieldAtlas(const ImageIter imgBegin, const ImageIter imgEnd, const Packing & packing,
                         const ImageTransform distanceTransform, const ImageTransform downSampling)
{
    internal::checkImageIteratorType<ImageIter>();
    assert(std::distance(imgBegin, imgEnd) == static_cast<typename std::iterator_traits<ImageIter>::difference_type>(packing.rects.size()));

    Image atlas{packing.atlasSize.x, packing.atlasSize.y, DistanceTransform::bitDepth};
    atlas.fillRect({0, 0}, atlas.getSize(), DistanceTransform::backgroundVal);

    auto rectIt = packing.rects.begin();
    for (auto it = imgBegin; it < imgEnd; rectIt++, ++it)
    {
        Image distField{it->getWidth(), it->getHeight(), DistanceTransform::bitDepth};
        distanceTransform(*it, distField);

        Image output = atlas.view(rectIt->position, rectIt->position + rectIt->size);
        downSampling(output, distField);
    }

    return atlas;
}


} // namespace llassetgen
