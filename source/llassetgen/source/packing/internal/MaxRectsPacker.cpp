#include <llassetgen/packing/internal/MaxRectsPacker.h>

#include <algorithm>
#include <iterator>
#include <limits>

using llassetgen::PackingSizeType;
using llassetgen::Rect;
using llassetgen::Vec2;

template <class T>
bool isInRange(T value, T min, T max) {
    return value > min && value < max;
}

/**
 * Replace a rect in a vector by one or more replacements.
 *
 * Reuses the replaced rect before pushing to the end of the vector.
 */
class RectReplacer {
   private:
    std::vector<Rect<PackingSizeType>>& vector;
    Rect<PackingSizeType>& existing;
    bool usedExisting = false;

   public:
    RectReplacer(std::vector<Rect<PackingSizeType>>& _vector, Rect<PackingSizeType>& _existing)
        : vector(_vector), existing(_existing){};

    void add_replacement(Rect<PackingSizeType>&& element);
};

void RectReplacer::add_replacement(Rect<PackingSizeType>&& element) {
    if (!usedExisting) {
        existing = element;
        usedExisting = true;
    } else {
        vector.push_back(element);
    }
}

/**
 * Crops a rectangle out of another, turning it into up to 4 remaining rectangles.
 *
 * The remaining rectangles have maximal width and height, and therefore
 * overlap.
 */
void cropRect(const Rect<PackingSizeType>& rect, const Rect<PackingSizeType>& bbox, RectReplacer replacer) {
    auto rectMin = rect.position;
    auto rectMax = rect.position + rect.size;
    auto bboxMin = bbox.position;
    auto bboxMax = bbox.position + bbox.size;

    if (bboxMin.x < rectMax.x && bboxMax.x > rectMin.x) {
        if (isInRange(bboxMin.y, rectMin.y, rectMax.y)) {
            replacer.add_replacement({rectMin, {rect.size.x, bboxMin.y - rectMin.y}});
        }

        if (isInRange(bboxMax.y, rectMin.y, rectMax.y)) {
            replacer.add_replacement({{rectMin.x, bboxMax.y}, {rect.size.x, rectMax.y - bboxMax.y}});
        }
    }

    if (bboxMin.y < rectMax.y && bboxMax.y > rectMin.y) {
        if (isInRange(bboxMin.x, rectMin.x, rectMax.x)) {
            replacer.add_replacement({rectMin, {bboxMin.x - rectMin.x, rect.size.y}});
        }

        if (isInRange(bboxMax.x, rectMin.x, rectMax.x)) {
            replacer.add_replacement({{bboxMax.x, rectMin.y}, {rectMax.x - bboxMax.x, rect.size.y}});
        }
    }
}

bool canContain(const Rect<PackingSizeType>& free, const Rect<PackingSizeType>& toBePlaced) {
    return free.size.x >= toBePlaced.size.x && free.size.y >= toBePlaced.size.y;
}

/**
 * Score how well a to be placed rectangle fits into a to a free one.
 *
 * Uses the best side short fit heuristic.
 */
PackingSizeType bssfScore(const Rect<PackingSizeType>& free, const Rect<PackingSizeType>& toBePlaced) {
    if (!canContain(free, toBePlaced)) {
        return std::numeric_limits<PackingSizeType>::max();
    }

    auto remainder = toBePlaced.size - free.size;
    return std::min(remainder.x, remainder.y);
}

namespace llassetgen {
    namespace internal {
        bool MaxRectsPacker::pack(Rect<PackingSizeType>& rect) {
            // TODO: Implement sorting of rectangles (DESCSS)
            // TODO: Implement growing and rotations
//            assert(!allowGrowth && !allowRotations);
            if (allowRotations || allowGrowth) {
                return true;
            }

            auto& freeRect =
                *std::min_element(freeList.begin(), freeList.end(),
                                  [&rect](const Rect<PackingSizeType>& r1, const Rect<PackingSizeType>& r2) {
                                      return bssfScore(r1, rect) < bssfScore(r2, rect);
                                  });

            if (!canContain(freeRect, rect)) {
                return false;
            }

            rect.position = freeRect.position;
            cropRects(rect);
            pruneFreeList();

            return true;
        }

        void MaxRectsPacker::pruneFreeList() {
            // Remove redundant rectangles by swapping them to the end of the vector
            // and resizing the vector when done.
            if (freeList.empty()) {
                return;
            }

            auto endIter = std::prev(freeList.end());
            for (auto iter1 = freeList.begin(); iter1 < endIter; ++iter1) {
                for (auto iter2 = std::next(iter1); iter2 <= endIter;) {
                    if (iter1->contains(*iter2)) {
                        std::iter_swap(iter2, endIter--);
                    } else if (iter2->contains(*iter1)) {
                        std::iter_swap(iter1, endIter--);
                        break;
                    } else {
                        ++iter2;
                    }
                }
            }

            freeList.resize(endIter - freeList.begin() + 1);
        }

        void MaxRectsPacker::cropRects(const Rect<PackingSizeType>& placedRect) {
            size_t originalRectCount = freeList.size();
            for (size_t i = 0; i < originalRectCount; ++i) {
                auto freeRectCopy = freeList[i];
                RectReplacer replacer{freeList, freeList[i]};
                cropRect(freeRectCopy, placedRect, replacer);
            }
        }
    }
}
