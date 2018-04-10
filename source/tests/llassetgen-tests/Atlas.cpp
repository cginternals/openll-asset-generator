#include <gmock/gmock.h>
#include <llassetgen/Atlas.h>

using namespace llassetgen;

std::string atlasTestDestinationPath = "../../";
std::vector<Vec2<size_t>> atlasTestSizes {{1, 1}, {34, 5}, {23, 79}, {16, 70}, {91, 64}, {98, 82}, {54, 63}, {100, 6}};

template <class DTType>
void createAtlas(std::vector<Image>& glyphs, Packing packing) {
    auto dtFunc = [](Image& in, Image& out) { DTType(in, out).transform(); };
    Image atlas = distanceFieldAtlas(glyphs.begin(), glyphs.end(), packing, dtFunc);

    std::string outPath = atlasTestDestinationPath + "dt_atlas.png";
    atlas.exportPng<DistanceTransform::OutputType>(outPath, -50, 50);
}

TEST(AtlasTest, CreateDistanceFieldAtlas) {
    std::vector<Image> glyphs;
    glyphs.reserve(atlasTestSizes.size());

    for (const auto& size : atlasTestSizes) {
        glyphs.emplace_back(size.x, size.y, 1);
    }

    auto dtFunc = [](Image& in, Image& out) {
        DeadReckoning(in, out).transform();
    };
    Packing p = shelfPackAtlas(atlasTestSizes.begin(), atlasTestSizes.end(), false);
    createAtlas<DeadReckoning>(glyphs, p);
    createAtlas<ParabolaEnvelope>(glyphs, p);
}

TEST(AtlasTest, CreateFontAtlas) {
    std::vector<Image> glyphs;
    glyphs.reserve(atlasTestSizes.size());

    for (const auto& size : atlasTestSizes) {
        glyphs.emplace_back(size.x, size.y, 1);
    }

    Packing p = shelfPackAtlas(atlasTestSizes.begin(), atlasTestSizes.end(), false);
    Image atlas = fontAtlas(glyphs.begin(), glyphs.end(), p);

    std::string outPath = atlasTestDestinationPath + "atlas.png";
    atlas.exportPng<uint8_t>(outPath);
}
