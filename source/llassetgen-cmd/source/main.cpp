#include <CLI11.h>
#include <codecvt>
#include <map>
#include <ostream>

#include "FontFinder.h"
#include <llassetgen/Atlas.h>

using namespace llassetgen;

std::map<std::string, std::function<std::unique_ptr<DistanceTransform>(Image&, Image&)>> dtFactory{
    std::make_pair("deadrec",
                   [](Image& input, Image& output) {
                       return std::unique_ptr<DistanceTransform>(new DeadReckoning(input, output));
                   }),
    std::make_pair("parabola",
                   [](Image& input, Image& output) {
                       return std::unique_ptr<DistanceTransform>(new ParabolaEnvelope(input, output));
                   }),
};

std::u32string UTF8toUCS4(const std::string& str) {
#if _MSC_VER >= 1900  // handle Visual Studio bug
    std::wstring_convert<std::codecvt_utf8<uint32_t>, uint32_t> ucs4conv;
    auto convStr = ucs4conv.from_bytes(str);
    return std::u32string(reinterpret_cast<const char32_t*>(convStr.data()), convStr.size());
#else
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> ucs4conv;
    return ucs4conv.from_bytes(str);
#endif
}

void distField(std::string& algorithm, Image& input, std::string& outPath) {
    Image output = Image(input.getWidth(), input.getHeight(), sizeof(DistanceTransform::OutputType) * 8);
    std::unique_ptr<DistanceTransform> dt = dtFactory[algorithm](input, output);
    dt->transform();
    output.exportPng<DistanceTransform::OutputType>(outPath, 20, -30);
}

std::vector<Vec2<size_t>> sizes(const std::vector<Image>& images, size_t padding = 0) {
    std::vector<Vec2<size_t>> imageSizes(images.size());
    std::transform(images.begin(), images.end(), imageSizes.begin(),
        [padding](const Image& img){ return img.getSize(); }
    );
    return imageSizes;
};

int main(int argc, char** argv) {
    llassetgen::init();

    CLI::App app{"OpenLL Font Asset Generator"};
    CLI::App* distfield = app.add_subcommand("distfield");
    CLI::App* atlas = app.add_subcommand("atlas");

    // distfield params
    std::string algorithm = "deadrec";  // default value
    std::set<std::string> algoOptions;
    for (const auto& algo : dtFactory) {
        algoOptions.insert(algo.first);
    }
    distfield->add_set("-a,--algorithm", algorithm, algoOptions);

    std::string glyphs;
    CLI::Option* glyphsOpt = distfield->add_option("-g,--glyph", glyphs);

    std::string fontPath;
    CLI::Option* fontPathOpt = distfield->add_option("--fontpath", fontPath)->check(CLI::ExistingFile);
    fontPathOpt->requires(glyphsOpt);

    std::string fontName;
    CLI::Option* fontNameOpt = distfield->add_option("--fontname,-f", fontName);
    fontNameOpt->requires(glyphsOpt);

    glyphsOpt->requires_one({fontPathOpt, fontNameOpt});

    int fontSize = 128;
    distfield->add_option("-s,--fontsize", fontSize)->requires(glyphsOpt);

    std::string imgPath;
    CLI::Option* imgOpt = distfield->add_option("--image,-i", imgPath)->check(CLI::ExistingFile);
    imgOpt->excludes(glyphsOpt);

    std::string outPath;
    distfield->add_option("outfile", outPath)->required();

    // atlas params
    // TODO

    CLI11_PARSE(app, argc, argv);

    if (app.got_subcommand(distfield)) {
        std::unique_ptr<Image> input;
        if (glyphsOpt->count()) {
            // Example: llassetgen-cmd distfield -a parabola -g G -f Verdana -s 64 glyph.png
            try {
                FontFinder fontFinder = fontPathOpt->count() ? FontFinder::fromPath(fontPath)
                                                             : FontFinder::fromName(fontName);

                auto ucs4Glyphs = UTF8toUCS4(glyphs);
                size_t padding = 20;
                std::vector<Image> glyphImages;
                fontFinder.renderGlyphs(ucs4Glyphs, glyphImages, fontSize, padding);
                std::vector<Vec2<size_t>> imageSizes = sizes(glyphImages);
                Packing p = shelfPackAtlas(imageSizes.begin(), imageSizes.end(), false);
                Image atlas = distanceFieldAtlas<ParabolaEnvelope>(glyphImages.begin(), glyphImages.end(), p);
                atlas.exportPng<DistanceTransform::OutputType>(outPath, 20, -30);
                return 0;
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return 2;
            }

        } else if (imgOpt->count()) {
            // Example: llassetgen-cmd distfield -a deadrec -i input.png output.png
            input = std::unique_ptr<Image>(new Image(imgPath));
        } else {
            return distfield->exit(CLI::CallForHelp());
        }

        if (!input) return 2;

        distField(algorithm, *input, outPath);
    } else if (app.got_subcommand(atlas)) {
        // TODO
    } else {
        return app.exit(CLI::CallForHelp());
    }

    return 0;
}
