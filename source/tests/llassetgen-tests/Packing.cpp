#include <gmock/gmock.h>

#include <numeric>

#include <llassetgen/llassetgen.h>
#include <llassetgen/packing/Types.h>
#include <llassetgen/packing/Algorithms.h>


using llassetgen::Packing;


using Vec = llassetgen::Vec2<llassetgen::PackingSizeType>;
using Rect = llassetgen::Rect<llassetgen::PackingSizeType>;


/**
 * Base class for all packing test fixtures.
 *
 * Defines several test methods that should be run for all packing methods.
 */
class PackingTest : public testing::Test {
   protected:
    virtual Packing run(const std::vector<Vec>& rectSizes, bool allowRotations, Vec atlasSize) = 0;
    virtual Packing run(const std::vector<Vec>& rectSizes, bool allowRotations) = 0;

    static bool rotatedSizesEquals(Vec size1, Vec size2) { return size1 == size2 || size1 == Vec{size2.y, size2.x}; }

    static bool doNotOverlap(const Rect& rect1, const Rect& rect2) { return !rect1.overlaps(rect2); }

    /**
     * Expect a fixed size packing to succeed and validate the result.
     */
    Packing expectSuccessfulValidPacking(const std::vector<Vec>& rectSizes, Vec atlasSize, bool allowRotations);

    /**
     * Expects a variable size packing to return a valid result.
     */
    Packing expectValidPacking(const std::vector<Vec>& rectSizes, bool allowRotations) {
        Packing packing = run(rectSizes, allowRotations);
        validatePacking(packing, rectSizes, allowRotations);
        return packing;
    }

    /**
     * Validate a packing given the input parameters.
     */
    void validatePacking(const Packing& packing, const std::vector<Vec>& rectSizes, bool allowRotations);

    void expectPackingFailure(const std::vector<Vec>& rectSizes, bool allowRotations, Vec atlasSize) {
        std::vector<Rect> emptyVec{};
        EXPECT_EQ(emptyVec, run(rectSizes, allowRotations, atlasSize).rects);
    }

    void testRejectTooWide() {
        expectPackingFailure({{2, 1}}, false, {1, 1});
        expectPackingFailure({{2, 1}}, true, {1, 1});
    }

    void testRejectTooHigh() {
        expectPackingFailure({{1, 2}}, false, {1, 1});
        expectPackingFailure({{1, 2}}, true, {1, 1});
    }

    void testRotateOnly() {
        expectPackingFailure({{2, 1}, {1, 2}}, false, {2, 2});
        expectSuccessfulValidPacking({{2, 1}, {1, 2}}, {2, 2}, true);
    }

    void testAcceptAtlasSized() {
        expectSuccessfulValidPacking({{1, 1}}, {1, 1}, false);
        expectSuccessfulValidPacking({{1, 1}}, {1, 1}, true);
    }

    void testAcceptMultipleTiny() {
        expectSuccessfulValidPacking({{1, 2}, {3, 4}, {5, 6}, {7, 8}}, {256, 256}, false);
        expectSuccessfulValidPacking({{1, 2}, {3, 4}, {5, 6}, {7, 8}}, {256, 256}, true);
    }

    void testVariableSizePacking() {
        expectValidPacking({{1, 2}, {3, 4}, {5, 6}}, false);
        expectValidPacking({{1, 2}, {3, 4}, {5, 6}}, true);
    }

    void testNonSquareSizePrediction() {
        auto expectValidNonSquarePacking = [this](const std::vector<Vec>& rectSizes, bool allowRotations) {
            Packing p = expectValidPacking(rectSizes, allowRotations);
            EXPECT_NE(p.atlasSize.x, p.atlasSize.y);
        };
        // Should not result in a square packing with any packing method.
        expectValidNonSquarePacking({{1, 1024}}, false);
        expectValidNonSquarePacking({{1, 1024}}, true);
        expectValidNonSquarePacking({{1024, 1}}, false);
        expectValidNonSquarePacking({{1024, 1}}, true);
    }

    void testTooSmallSizePrediction() {
        // These have a combined area of 28 < 4*8, but can't be packed into a
        // 4x8 atlas. Testing both direction forces both directions of growth
        // in packing methods that support it.
        std::vector<Vec> rectSizes{{8, 2}, {8, 1}, {1, 4}};
        std::vector<Vec> rectSizesRotated{{2, 8}, {1, 8}, {4, 1}};
        expectValidPacking(rectSizes, false);
        expectValidPacking(rectSizes, true);
        expectValidPacking(rectSizesRotated, false);
        expectValidPacking(rectSizesRotated, true);
    }
};

Packing PackingTest::expectSuccessfulValidPacking(const std::vector<Vec>& rectSizes, Vec atlasSize,
                                                  bool allowRotations) {
    Packing packing = run(rectSizes, allowRotations, atlasSize);
    EXPECT_EQ(atlasSize, packing.atlasSize);
    validatePacking(packing, rectSizes, allowRotations);
    return packing;
}

void PackingTest::validatePacking(const Packing& packing, const std::vector<Vec>& rectSizes, bool allowRotations) {
    EXPECT_EQ(rectSizes.size(), packing.rects.size());

    for (size_t i = 0; i < packing.rects.size(); i++) {
        auto rect = packing.rects[i];
        if (allowRotations) {
            EXPECT_PRED2(rotatedSizesEquals, rectSizes[i], rect.size);
        } else {
            EXPECT_EQ(rectSizes[i], rect.size);
        }

        EXPECT_LE(0, rect.position.x);
        EXPECT_LE(0, rect.position.y);
        EXPECT_GE(packing.atlasSize.x, rect.position.x + rect.size.x);
        EXPECT_GE(packing.atlasSize.y, rect.position.y + rect.size.y);

        for (size_t j = i + 1; j < packing.rects.size(); j++) {
            EXPECT_PRED2(doNotOverlap, rect, packing.rects[j]);
        }
    }
}

class ShelfNextFitPackingTest : public PackingTest {
    Packing run(const std::vector<Vec>& rectSizes, bool allowRotations, Vec atlasSize) override {
        return llassetgen::shelfPackAtlas(rectSizes.begin(), rectSizes.end(), atlasSize, allowRotations);
    }

    Packing run(const std::vector<Vec>& rectSizes, bool allowRotations) override {
        return llassetgen::shelfPackAtlas(rectSizes.begin(), rectSizes.end(), allowRotations);
    }
};

class MaxRectsPackingTest : public PackingTest {
   protected:
    Packing run(const std::vector<Vec>& rectSizes, bool allowRotations, Vec atlasSize) override {
        return llassetgen::maxRectsPackAtlas(rectSizes.begin(), rectSizes.end(), atlasSize, allowRotations);
    }

    Packing run(const std::vector<Vec>& rectSizes, bool allowRotations) override {
        return llassetgen::maxRectsPackAtlas(rectSizes.begin(), rectSizes.end(), allowRotations);
    }

   public:
    void testNoFreeRect() {
        // No free rect available when packing the second rect.
        expectPackingFailure({{1, 1}, {1, 1}}, false, {1, 1});
        expectPackingFailure({{1, 1}, {1, 1}}, false, {1, 1});

        // Uses internal packer because chosen atlas size always has enough
        // area for all rectangles.
        for (bool allowRotations : {false, true}) {
            llassetgen::internal::MaxRectsPacker packer{{1, 1}, allowRotations, true};
            Rect r1{{0, 0}, {1, 1}};
            Rect r2{{0, 0}, {1, 1}};
            EXPECT_TRUE(packer.pack(r1));
            EXPECT_TRUE(packer.pack(r2));
            EXPECT_TRUE(packer.atlasSize().x > 1 || packer.atlasSize().y > 1);
        }
    }

    void testFreeRectPruning() {
        // Test case forces the following to happen:
        //  - Rectangle placed top-left, creating two free rectangles which
        //    overlap in the bottom-right
        //  - Fill the non-overlapping part of one rectangle, causing the
        //    remainder to be pruned. Chosen due to best short-side fit.
        //  - Fill the other rectangle completely. Chosen because only it can
        //    fit the rectangle
        //  - Last rectangle is the size of the pruned rectangle, and should
        //    fail to pack
        //
        // To avoid reordering of the rectangles, this test uses the internal
        // packing class directly.
        auto test = [](const std::vector<Vec>& rectSizes, const Vec& nonFitting) {
            llassetgen::internal::MaxRectsPacker packer{{8, 8}, false, false};
            for (Vec size : rectSizes) {
                Rect r{{0, 0}, size};
                EXPECT_TRUE(packer.pack(r));
            }

            Rect r{{0, 0}, nonFitting};
            EXPECT_FALSE(packer.pack(r));
        };

        // Test both orientations
        test({{4, 3}, {4, 3}, {8, 5}}, {4, 5});
        test({{3, 4}, {3, 4}, {5, 8}}, {5, 4});
    }
};

#define ADD_TESTS_FOR_FIXTURE(Fixture)                                              \
    TEST_F(Fixture, TestRejectTooWide) { testRejectTooWide(); }                     \
    TEST_F(Fixture, TestRejectTooHigh) { testRejectTooHigh(); }                     \
    TEST_F(Fixture, TestRotateOnly) { testRotateOnly(); }                           \
    TEST_F(Fixture, TestAcceptAtlasSized) { testAcceptAtlasSized(); }               \
    TEST_F(Fixture, TestAcceptMultipleTiny) { testAcceptMultipleTiny(); }           \
    TEST_F(Fixture, TestVariableSizePacking) { testVariableSizePacking(); }         \
    TEST_F(Fixture, TestNonSquareSizePrediction) { testNonSquareSizePrediction(); } \
    TEST_F(Fixture, TestTooSmallSizePrediction) { testTooSmallSizePrediction(); }

ADD_TESTS_FOR_FIXTURE(ShelfNextFitPackingTest)
ADD_TESTS_FOR_FIXTURE(MaxRectsPackingTest)

#undef ADD_TESTS_FOR_FIXTURE

TEST_F(MaxRectsPackingTest, TestNoFreeRect) { testNoFreeRect(); }
TEST_F(MaxRectsPackingTest, TestFreeRectPruning) { testFreeRectPruning(); }

TEST(PackingInternalsTest, TestCeilLog2) {
    for (int i = 0; i < 64; i++) {
        std::uint64_t twoToTheI = static_cast<std::uint64_t>(1) << i;
        EXPECT_EQ(i, llassetgen::internal::ceilLog2(twoToTheI));
        EXPECT_EQ(i + 1, llassetgen::internal::ceilLog2(twoToTheI + 1));
        if (twoToTheI > 2) {
            EXPECT_EQ(i, llassetgen::internal::ceilLog2(twoToTheI - 1));
        }
    }

    EXPECT_EQ(64, llassetgen::internal::ceilLog2(std::numeric_limits<std::uint64_t>::max()));
}
