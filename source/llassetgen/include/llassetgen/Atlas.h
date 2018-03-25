#pragma once

#include <llassetgen/Image.h>
#include <llassetgen/Packing.h>
#include <llassetgen/DistanceTransform.h>

namespace llassetgen {
    namespace internal {
        template<class Iter>
        constexpr int checkImageIteratorType() {
            using IterTraits = typename std::iterator_traits<Iter>;
            using IterRefType = typename IterTraits::reference;
            using IterCategory = typename IterTraits::iterator_category;
            static_assert(std::is_assignable<Image, IterRefType>::value,
                          "Input elements must be assignable to Image");
            static_assert(std::is_base_of<std::input_iterator_tag, IterCategory>::value,
                          "Input iterator must be an InputIterator");
            return 0;
        }
    }

    template <class ImageIter>
    Image fontAtlas(ImageIter imgBegin, ImageIter imgEnd, Packing packing, size_t padding, uint8_t bitDepth) {
        internal::checkImageIteratorType<ImageIter>();
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
        internal::checkImageIteratorType<ImageIter>();
        static_assert(std::is_base_of<DistanceTransform, DTType>::value,
                      "DTType must be a DistanceTransform");
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
