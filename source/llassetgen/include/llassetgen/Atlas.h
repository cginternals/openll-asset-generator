#pragma once

#include <llassetgen/DistanceTransform.h>
#include <llassetgen/Image.h>
#include <llassetgen/Packing.h>

namespace llassetgen {
    using DTFunc = void (*) (Image&, Image&);

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

    template <class ImageIter>
    Image distanceFieldAtlas(ImageIter imgBegin, ImageIter imgEnd, Packing packing, DTFunc dtFunc) {
        using DiffType = typename std::iterator_traits<ImageIter>::difference_type;
        assert(std::distance(imgBegin, imgEnd) == static_cast<DiffType>(packing.rects.size()));

        Image atlas{packing.atlasSize.x, packing.atlasSize.y, DistanceTransform::bitDepth};
        atlas.fillRect({0, 0}, atlas.getSize(), DistanceTransform::backgroundVal);

        auto rectIt = packing.rects.begin();
        for (; imgBegin < imgEnd; rectIt++, imgBegin++) {
            Image distField{imgBegin->getWidth(), imgBegin->getHeight(), DistanceTransform::bitDepth};
            dtFunc(*imgBegin, distField);

            Image output = atlas.view(rectIt->position, rectIt->position + rectIt->size);
            output.centerDownsampling<float>(distField);
        }

        return atlas;
    }
}
