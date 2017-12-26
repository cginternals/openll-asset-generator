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
    virtual boost::optional<Packing> run(std::vector<Vec> rectSizes, Vec atlasSize) = 0;

    /**
     * Expect a packing to succeed and validate the result.
     */
    void validatePacking(std::vector<Vec> rectSizes, Vec atlasSize) {
        auto maybePacking = run(rectSizes, atlasSize);
        ASSERT_NE(boost::none, maybePacking);
        Packing packing = *std::move(maybePacking);

        EXPECT_EQ(rectSizes.size(), packing.rects.size());
        EXPECT_EQ(atlasSize, packing.atlasSize);

        for (size_t i = 0; i < packing.rects.size(); i++) {
            auto rect = packing.rects[i];
            EXPECT_EQ(rectSizes[i], rect.size);
            EXPECT_LE(0, rect.position.x);
            EXPECT_LE(0, rect.position.y);
            EXPECT_GE(packing.atlasSize.x, rect.position.x + rect.size.x);
            EXPECT_GE(packing.atlasSize.y, rect.position.y + rect.size.y);
        }
    }

    void testRejectTooWide() { EXPECT_EQ(boost::none, run({{2, 1}}, {1, 1})); }

    void testRejectTooHigh() { EXPECT_EQ(boost::none, run({{1, 2}}, {1, 1})); }

    void testRejectImpossible() { EXPECT_EQ(boost::none, run({{2, 1}, {1, 2}}, {2, 2})); }

    void testAcceptAtlasSized() { validatePacking({{1, 1}}, {1, 1}); }

    void testAcceptMultipleTiny() { validatePacking({{1, 2}, {3, 4}, {5, 6}, {7, 8}}, {256, 256}); }
};

class ShelfNextFitPackingTest : public PackingTest {
    boost::optional<Packing> run(std::vector<Vec> rectSizes, Vec atlasSize) override {
        return llassetgen::shelfPackAtlas(rectSizes.begin(), rectSizes.end(), atlasSize);
    }
};

TEST_F(ShelfNextFitPackingTest, TestRejectTooWide) { testRejectTooWide(); }

TEST_F(ShelfNextFitPackingTest, TestRejectTooHigh) { testRejectTooHigh(); }

TEST_F(ShelfNextFitPackingTest, TestRejectImpossible) { testRejectImpossible(); }

TEST_F(ShelfNextFitPackingTest, TestAcceptAtlasSized) { testAcceptAtlasSized(); }

TEST_F(ShelfNextFitPackingTest, TestAcceptMultipleTiny) { testAcceptMultipleTiny(); }
