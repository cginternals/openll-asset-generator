#include <gmock/gmock.h>
#include <llassetgen/Atlas.h>

using namespace llassetgen;

std::string atlas_test_destination_path = "../../";

TEST(AtlasTest, CreateDistanceFieldAtlas) {
    std::vector<Vec2<size_t>> sizes {{1, 1}};
    std::vector<Image> glyphs;

    for (const auto& size : sizes) {
        glyphs.emplace_back(size.x, size.y, 1);
    }

    Packing p = shelfPackAtlas(sizes.begin(), sizes.end(), false);
    Image atlas = distanceFieldAtlas<DeadReckoning>(glyphs.begin(), glyphs.end(), p);

    std::string outPath = atlas_test_destination_path + "dt_atlas.png";
    atlas.exportPng<DistanceTransform::OutputType>(outPath, -50, 50);
}
