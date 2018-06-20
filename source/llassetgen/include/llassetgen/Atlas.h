#pragma once

#include <llassetgen/DistanceTransform.h>
#include <llassetgen/Image.h>
#include <llassetgen/Packing.h>

namespace llassetgen {
    using ImageTransform = void (*)(Image&, Image&);

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

#pragma omp parallel for
        for (int i = 0; i < std::distance(imgBegin, imgEnd); i++) {
            auto& rect = packing.rects[i];
            Image view = atlas.view(rect.position, rect.position + rect.size);
            view.copyDataFrom(imgBegin[i]);
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
        internal::checkImageIteratorType<ImageIter>();
        using DiffType = typename std::iterator_traits<ImageIter>::difference_type;
        assert(std::distance(imgBegin, imgEnd) == static_cast<DiffType>(packing.rects.size()));

        Image atlas{packing.atlasSize.x, packing.atlasSize.y, DistanceTransform::bitDepth};
        atlas.fillRect({0, 0}, atlas.getSize(), DistanceTransform::backgroundVal);

        const int max = std::distance(imgBegin, imgEnd);
#pragma omp parallel for
        for (int i = 0; i < max; i++) {
            auto& imgInput = imgBegin[i];
            Image distField{imgInput.getWidth(), imgInput.getHeight(), DistanceTransform::bitDepth};
            distanceTransform(imgInput, distField);

            auto& rect = packing.rects[i];
            Image output = atlas.view(rect.position, rect.position + rect.size);
            downSampling(output, distField);
        }

        return atlas;
    }
}
