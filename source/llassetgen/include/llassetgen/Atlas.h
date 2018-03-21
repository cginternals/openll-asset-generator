#pragma once

#include <llassetgen/Image.h>
#include <llassetgen/Packing.h>

namespace llassetgen {
    template <class ImageIter>
    Image fontAtlas(ImageIter imgBegin, ImageIter imgEnd, Packing packing, size_t padding, uint8_t bitDepth) {
        assert(std::distance(imgBegin, imgEnd) == static_cast<ptrdiff_t>(packing.rects.size()));

        Image atlas {packing.atlasSize.x, packing.atlasSize.y, bitDepth};
        atlas.clear();

        auto rectIt = packing.rects.begin();
        for (; imgBegin < imgEnd; rectIt++, imgBegin++) {
            Image view = atlas.view(rectIt->position, rectIt->position + rectIt->size, padding);
            view.load(*imgBegin);
        }
        return atlas;
    }

    template<class DTType, class ImageIter>
    Image distanceFieldAtlas(ImageIter imgBegin, ImageIter imgEnd, Packing packing) {
        assert(std::distance(imgBegin, imgEnd) == static_cast<ptrdiff_t>(packing.rects.size()));

        Image atlas {packing.atlasSize.x, packing.atlasSize.y, DistanceTransform::bitDepth};
        atlas.fillRect({0,0}, atlas.getSize(), DistanceTransform::backgroundVal);

        auto rectIt = packing.rects.begin();
        for (; imgBegin < imgEnd; rectIt++, imgBegin++) {
            Image output = atlas.view(rectIt->position, rectIt-> position + rectIt->size);
            DTType(*imgBegin, output).transform();
        }

        return atlas;
    }
}
