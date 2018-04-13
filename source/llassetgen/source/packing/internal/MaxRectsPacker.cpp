#include <llassetgen/packing/internal/MaxRectsPacker.h>

#include <iterator>
#include <limits>

using llassetgen::PackingSizeType;
using llassetgen::Rect;

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

    void addReplacement(Rect<PackingSizeType>&& element);
};

void RectReplacer::addReplacement(Rect<PackingSizeType>&& element) {
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
            replacer.addReplacement({rectMin, {rect.size.x, bboxMin.y - rectMin.y}});
        }

        if (isInRange(bboxMax.y, rectMin.y, rectMax.y)) {
            replacer.addReplacement({{rectMin.x, bboxMax.y}, {rect.size.x, rectMax.y - bboxMax.y}});
        }
    }

    if (bboxMin.y < rectMax.y && bboxMax.y > rectMin.y) {
        if (isInRange(bboxMin.x, rectMin.x, rectMax.x)) {
            replacer.addReplacement({rectMin, {bboxMin.x - rectMin.x, rect.size.y}});
        }

        if (isInRange(bboxMax.x, rectMin.x, rectMax.x)) {
            replacer.addReplacement({{bboxMax.x, rectMin.y}, {rectMax.x - bboxMax.x, rect.size.y}});
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

class BssfComparator {
   public:
    explicit BssfComparator(const Rect<PackingSizeType>& _toBePlaced) : toBePlaced(_toBePlaced) {}

    bool operator()(const Rect<PackingSizeType>& free1, const Rect<PackingSizeType>& free2) {
        return bssfScore(free1, toBePlaced) < bssfScore(free2, toBePlaced);
    }

   private:
    const Rect<PackingSizeType>& toBePlaced;
};

namespace llassetgen {
    namespace internal {
        bool MaxRectsPacker::inputSortingComparator(const Rect<PackingSizeType>& rect1,
                                                    const Rect<PackingSizeType>& rect2) {
            // Sort by shortest side fit descending (DESCSS)
            return std::minmax(rect1.size.x, rect1.size.y) > std::minmax(rect2.size.x, rect2.size.y);
        }

        bool MaxRectsPacker::pack(Rect<PackingSizeType>& rect) {
            auto freeRectIter = findFreeRect(rect);
            if (allowGrowth) {
                while (freeRectIter == freeList.end() || !canContain(*freeRectIter, rect)) {
                    grow();
                    freeRectIter = findFreeRect(rect);
                }
            } else {
                if (freeRectIter == freeList.end() || !canContain(*freeRectIter, rect)) {
                    return false;
                }
            }

            rect.position = freeRectIter->position;
            cropRects(rect);
            pruneFreeList();

            return true;
        }

        void MaxRectsPacker::grow() {
            if (atlasSize_.x > atlasSize_.y) {
                for (auto& freeRect : freeList) {
                    if (freeRect.position.y + freeRect.size.y == atlasSize_.y) {
                        freeRect.size.y += atlasSize_.y;
                    }
                }

                freeList.push_back({{0, atlasSize_.y}, atlasSize_});
                atlasSize_.y *= 2;
            } else {
                for (auto& freeRect : freeList) {
                    if (freeRect.position.x + freeRect.size.x == atlasSize_.x) {
                        freeRect.size.x += atlasSize_.x;
                    }
                }

                freeList.push_back({{atlasSize_.x, 0}, atlasSize_});
                atlasSize_.x *= 2;
            }
        }

        std::vector<Rect<PackingSizeType>>::const_iterator MaxRectsPacker::findFreeRect(
            Rect<PackingSizeType>& rect) const {
            if (freeList.empty()) {
                return freeList.end();
            }

            auto freeRectIter = std::min_element(freeList.begin(), freeList.end(), BssfComparator{rect});
            if (allowRotations) {
                Rect<PackingSizeType> rectRotated{rect.position, {rect.size.y, rect.size.x}};
                auto freeRectRotatedIter =
                    std::min_element(freeList.begin(), freeList.end(), BssfComparator{rectRotated});
                if (bssfScore(*freeRectRotatedIter, rectRotated) < bssfScore(*freeRectIter, rect)) {
                    rect.size = rectRotated.size;
                    return freeRectRotatedIter;
                }
            }

            return freeRectIter;
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
            size_t rectsToCrop = freeList.size();
            for (size_t i = 0; i < rectsToCrop;) {
                if (placedRect == freeList[i]) {
                    std::swap(freeList[i], freeList[freeList.size() - 1]);
                    freeList.resize(freeList.size() - 1);
                    --rectsToCrop;
                    // Is the swapped rectangle already created due to a crop?
                    if (freeList.size() != rectsToCrop) {
                        ++i;
                    }
                } else {
                    auto freeRectCopy = freeList[i];
                    RectReplacer replacer{freeList, freeList[i]};
                    cropRect(freeRectCopy, placedRect, replacer);
                    ++i;
                }
            }
        }
    }
}
