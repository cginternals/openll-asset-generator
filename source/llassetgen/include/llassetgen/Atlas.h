#pragma once

#include <llassetgen/DistanceTransform.h>
#include <llassetgen/Image.h>
#include <llassetgen/Packing.h>

namespace llassetgen {
    using ImageTransform = void (*)(Image&, Image&);

    template <class ImageIter>
    Image fontAtlas(ImageIter imgBegin, ImageIter imgEnd, Packing packing, uint8_t bitDepth = 1) {
        using DiffType = typename std::iterator_traits<ImageIter>::difference_type;
        assert(std::distance(imgBegin, imgEnd) == static_cast<DiffType>(packing.rects.size()));

        Image atlas{packing.atlasSize.x, packing.atlasSize.y, bitDepth};
        atlas.clear();

        auto rectIt = packing.rects.begin();
        for (; imgBegin < imgEnd; rectIt++, imgBegin++) {
            Image view = atlas.view(rectIt->position, rectIt->position + rectIt->size);
            view.copyDataFrom(*imgBegin);
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
    Image distanceFieldAtlas(ImageIter imgBegin, ImageIter imgEnd, Packing packing, ImageTransform distanceTransform,
                             ImageTransform downSampling) {
        using DiffType = typename std::iterator_traits<ImageIter>::difference_type;
        assert(std::distance(imgBegin, imgEnd) == static_cast<DiffType>(packing.rects.size()));

        Image atlas{packing.atlasSize.x, packing.atlasSize.y, DistanceTransform::bitDepth};
        atlas.fillRect({0, 0}, atlas.getSize(), DistanceTransform::backgroundVal);

        auto rectIt = packing.rects.begin();
        for (; imgBegin < imgEnd; rectIt++, imgBegin++) {
            Image distField{imgBegin->getWidth(), imgBegin->getHeight(), DistanceTransform::bitDepth};
            distanceTransform(*imgBegin, distField);

            Image output = atlas.view(rectIt->position, rectIt->position + rectIt->size);
            downSampling(output, distField);
        }

        return atlas;
    }
}

