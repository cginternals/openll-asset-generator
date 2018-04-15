#pragma once

#include <llassetgen/DistanceTransform.h>
#include <llassetgen/Image.h>
#include <llassetgen/Packing.h>

namespace llassetgen {
    using ImageTransform = void (*) (Image&, Image&);

    namespace internal {
        template <class Iter>
        constexpr int checkImageIteratorType() {
            using IterTraits = typename std::iterator_traits<Iter>;
            using IterType = typename IterTraits::value_type;
            using IterCategory = typename IterTraits::iterator_category;
            static_assert(std::is_assignable<Image, IterType>::value, "Input elements must be assignable to Image");
            static_assert(std::is_base_of<std::random_access_iterator_tag, IterCategory>::value,
                          "Input iterator must be a RandomAccessIterator");
            return 0;
        }
    }

    template <class ImageIter>
    Image fontAtlas(ImageIter imgBegin, ImageIter imgEnd, Packing packing, uint8_t bitDepth = 1) {
        internal::checkImageIteratorType<ImageIter>();
        using DiffType = typename std::iterator_traits<ImageIter>::difference_type;
        assert(std::distance(imgBegin, imgEnd) == static_cast<DiffType>(packing.rects.size()));

        Image atlas{packing.atlasSize.x, packing.atlasSize.y, bitDepth};
        atlas.clear();

        for (int i = 0; i < std::distance(imgBegin, imgEnd); i++) {
            auto& rect = packing.rects[i];
            Image view = atlas.view(rect.position, rect.position + rect.size);
            view.copyDataFrom(imgBegin[i]);
        }
        return atlas;
    }

    template <class ImageIter>
    Image distanceFieldAtlas(ImageIter imgBegin, ImageIter imgEnd, Packing packing, ImageTransform distanceTransform, ImageTransform downSampling) {
        internal::checkImageIteratorType<ImageIter>();
        using DiffType = typename std::iterator_traits<ImageIter>::difference_type;
        assert(std::distance(imgBegin, imgEnd) == static_cast<DiffType>(packing.rects.size()));

        Image atlas{packing.atlasSize.x, packing.atlasSize.y, DistanceTransform::bitDepth};
        atlas.fillRect({0, 0}, atlas.getSize(), DistanceTransform::backgroundVal);

        for (int i = 0; i < std::distance(imgBegin, imgEnd); i++) {
            Image distField{imgBegin[i].getWidth(), imgBegin[i].getHeight(), DistanceTransform::bitDepth};
            distanceTransform(imgBegin[i], distField);

            auto& rect = packing.rects[i];
            Image output = atlas.view(rect.position, rect.position + rect.size);
            downSampling(output, distField);
        }

        return atlas;
    }
}
