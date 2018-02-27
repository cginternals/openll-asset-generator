#include <gmock/gmock.h>

#include <numeric>

#include <llassetgen/llassetgen.h>

using llassetgen::Packing;

using Vec = llassetgen::Vec2<llassetgen::PackingSizeType>;

/**
 * Base class for all packing test fixtures.
 *
 * Defines several test methods that should be run for all packing methods.
 */
class PackingTest : public testing::Test {
   protected:
    virtual bool run(const std::vector<Vec>& rectSizes, bool allowRotations, Packing& packing) = 0;
    virtual Packing run(const std::vector<Vec>& rectSizes, bool allowRotations) = 0;

    static bool rotatedSizesEquals(Vec size1, Vec size2) { return size1 == size2 || size1 == Vec{size2.y, size2.x}; }

    /**
     * Expect a fixed size packing to succeed and validate the result.
     */
    void expectSuccessfulValidPacking(const std::vector<Vec>& rectSizes, Vec atlasSize, bool allowRotations);

    /**
     * Expects a variable size packing to return a valid result.
     */
    void expectValidPacking(const std::vector<Vec>& rectSizes, bool allowRotations) {
        validatePacking(run(rectSizes, allowRotations), rectSizes, allowRotations);
    }

    /**
     * Validate a packing given the input parameters.
     */
    void validatePacking(Packing packing, const std::vector<Vec>& rectSizes, bool allowRotations);

    void testRejectTooWide() {
        Packing p1{{1, 1}}, p2{{1, 1}};
        EXPECT_FALSE(run({{2, 1}}, false, p1));
        EXPECT_FALSE(run({{2, 1}}, true, p2));
    }

    void testRejectTooHigh() {
        Packing p1{{1, 1}}, p2{{1, 1}};
        EXPECT_FALSE(run({{1, 2}}, false, p1));
        EXPECT_FALSE(run({{1, 2}}, true, p2));
    }

    void testRotateOnly() {
        Packing p{{2, 2}};
        EXPECT_FALSE(run({{2, 1}, {1, 2}}, false, p));
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
};

void PackingTest::expectSuccessfulValidPacking(const std::vector<Vec>& rectSizes, Vec atlasSize, bool allowRotations) {
    Packing packing{atlasSize};
    ASSERT_TRUE(run(rectSizes, allowRotations, packing));
    EXPECT_EQ(atlasSize, packing.atlasSize);
    validatePacking(packing, rectSizes, allowRotations);
}

void PackingTest::validatePacking(Packing packing, const std::vector<Vec>& rectSizes, bool allowRotations) {
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
    }
}

class ShelfNextFitPackingTest : public PackingTest {
    bool run(const std::vector<Vec>& rectSizes, bool allowRotations, Packing& packing) override {
        return llassetgen::shelfPackFixedSizeAtlas(rectSizes.begin(), rectSizes.end(), allowRotations, packing);
    }

    Packing run(const std::vector<Vec>& rectSizes, bool allowRotations) override {
        return llassetgen::shelfPackAtlas(rectSizes.begin(), rectSizes.end(), allowRotations);
    }
};

TEST_F(ShelfNextFitPackingTest, TestRejectTooWide) { testRejectTooWide(); }
TEST_F(ShelfNextFitPackingTest, TestRejectTooHigh) { testRejectTooHigh(); }
TEST_F(ShelfNextFitPackingTest, TestRotateOnly) { testRotateOnly(); }
TEST_F(ShelfNextFitPackingTest, TestAcceptAtlasSized) { testAcceptAtlasSized(); }
TEST_F(ShelfNextFitPackingTest, TestAcceptMultipleTiny) { testAcceptMultipleTiny(); }
TEST_F(ShelfNextFitPackingTest, TestVariableSizePacking) { testVariableSizePacking(); }

TEST(TestPackingInternals, TestCeilLog2) {
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
