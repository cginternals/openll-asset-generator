#include <gmock/gmock.h>
#include <boost/optional.hpp>

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
    virtual boost::optional<Packing> run(const std::vector<Vec>& rectSizes, Vec atlasSize, bool allowRotations) = 0;
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
        EXPECT_EQ(boost::none, run({{2, 1}}, {1, 1}, false));
        EXPECT_EQ(boost::none, run({{2, 1}}, {1, 1}, true));
    }

    void testRejectTooHigh() {
        EXPECT_EQ(boost::none, run({{1, 2}}, {1, 1}, false));
        EXPECT_EQ(boost::none, run({{1, 2}}, {1, 1}, true));
    }

    void testRotateOnly() {
        EXPECT_EQ(boost::none, run({{2, 1}, {1, 2}}, {2, 2}, false));
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
    auto maybePacking = run(rectSizes, atlasSize, allowRotations);
    ASSERT_NE(boost::none, maybePacking);

    Packing packing = *maybePacking;
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
    boost::optional<Packing> run(const std::vector<Vec>& rectSizes, Vec atlasSize, bool allowRotations) override {
        return llassetgen::shelfPackAtlas(rectSizes.begin(), rectSizes.end(), atlasSize, allowRotations);
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
