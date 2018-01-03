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
    virtual boost::optional<Packing> run(std::vector<Vec> rectSizes, Vec atlasSize, bool allowRotations) = 0;

    static bool rotatedSizesEquals(Vec size1, Vec size2) { return size1 == size2 || size1 == Vec{size2.y, size2.x}; }

    /**
     * Expect a packing to succeed and validate the result.
     */
    void validatePacking(std::vector<Vec> rectSizes, Vec atlasSize, bool allowRotations) {
        auto maybePacking = run(rectSizes, atlasSize, allowRotations);
        ASSERT_NE(boost::none, maybePacking);
        Packing packing = *std::move(maybePacking);

        EXPECT_EQ(rectSizes.size(), packing.rects.size());
        EXPECT_EQ(atlasSize, packing.atlasSize);

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
        validatePacking({{2, 1}, {1, 2}}, {2, 2}, true);
    }

    void testAcceptAtlasSized() {
        validatePacking({{1, 1}}, {1, 1}, false);
        validatePacking({{1, 1}}, {1, 1}, true);
    }

    void testAcceptMultipleTiny() {
        validatePacking({{1, 2}, {3, 4}, {5, 6}, {7, 8}}, {256, 256}, false);
        validatePacking({{1, 2}, {3, 4}, {5, 6}, {7, 8}}, {256, 256}, true);
    }
};

class ShelfNextFitPackingTest : public PackingTest {
    boost::optional<Packing> run(std::vector<Vec> rectSizes, Vec atlasSize, bool allowRotations) override {
        return llassetgen::shelfPackAtlas(rectSizes.begin(), rectSizes.end(), atlasSize, allowRotations);
    }
};

TEST_F(ShelfNextFitPackingTest, TestRejectTooWide) { testRejectTooWide(); }

TEST_F(ShelfNextFitPackingTest, TestRejectTooHigh) { testRejectTooHigh(); }

TEST_F(ShelfNextFitPackingTest, TestRotateOnly) { testRotateOnly(); }

TEST_F(ShelfNextFitPackingTest, TestAcceptAtlasSized) { testAcceptAtlasSized(); }

TEST_F(ShelfNextFitPackingTest, TestAcceptMultipleTiny) { testAcceptMultipleTiny(); }
