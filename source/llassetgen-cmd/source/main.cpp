#include <CLI11.h>
#include <codecvt>
#include <map>
#include <ostream>

#include "FontFinder.h"
#include <llassetgen/Atlas.h>

using namespace llassetgen;

std::map<std::string, std::function<std::unique_ptr<DistanceTransform>(Image&, Image&)>> dtFactory {
    {
        "deadrec",
        [](Image &input, Image &output) {
            return std::unique_ptr<DistanceTransform>(new DeadReckoning(input, output));
        }
    },
    {
        "parabola",
        [](Image& input, Image& output) {
            return std::unique_ptr<DistanceTransform>(new ParabolaEnvelope(input, output));
        }
    }
};

std::set<std::string> algoOptions() {
    std::set<std::string> options;
    for (const auto& algo : dtFactory) {
        options.insert(algo.first);
    }
    return options;
}

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

std::vector<Vec2<size_t>> sizes(const std::vector<Image>& images, size_t padding = 0) {
    std::vector<Vec2<size_t>> imageSizes(images.size());
    std::transform(images.begin(), images.end(), imageSizes.begin(),
        [padding](const Image& img){ return img.getSize(); }
    );
    return imageSizes;
};

int parseAtlas(int argc, char **argv) {
    CLI::App app{"OpenLL Font Asset Generator"};

    std::string algorithm = "deadrec";  // default value
    CLI::Option* distfieldOpt = app.add_set("-d,--distfield", algorithm, algoOptions());

    std::string glyphs;
    CLI::Option* glyphsOpt = app.add_option("-g,--glyph", glyphs);

    std::string fontPath;
    CLI::Option* fontPathOpt = app.add_option("--fontpath", fontPath)->check(CLI::ExistingFile);

    std::string fontName;
    CLI::Option* fontNameOpt = app.add_option("--fontname,-f", fontName);

    glyphsOpt->requires_one({fontPathOpt, fontNameOpt});

    int fontSize = 128;
    app.add_option("-s,--fontsize", fontSize);

    std::string outPath;
    app.add_option("outfile", outPath)->required();

    CLI11_PARSE(app, argc, argv);

    // Example: llassetgen-cmd atlas -d parabola -g ABC -f Verdana -s 64 atlas.png
    try {
        FontFinder fontFinder = fontPathOpt->count() ? FontFinder::fromPath(fontPath)
                                                     : FontFinder::fromName(fontName);

        auto ucs4Glyphs = UTF8toUCS4(glyphs);
        size_t padding = 20;
        std::vector<Image> glyphImages;
        fontFinder.renderGlyphs(ucs4Glyphs, glyphImages, fontSize, padding);
        std::vector<Vec2<size_t>> imageSizes = sizes(glyphImages);
        Packing p = shelfPackAtlas(imageSizes.begin(), imageSizes.end(), false);

        if (distfieldOpt->count()) {
            Image atlas = distanceFieldAtlas<ParabolaEnvelope>(glyphImages.begin(), glyphImages.end(), p);
            atlas.exportPng<DistanceTransform::OutputType>(outPath, 20, -30);
        } else {
            Image atlas = fontAtlas(glyphImages.begin(), glyphImages.end(), p);
            atlas.exportPng<uint8_t>(outPath);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }

    return 0;
}

int parseDistField(int argc, char** argv) {
    CLI::App app{"OpenLL Font Asset Generator"};
    // Example: llassetgen-cmd distfield -a deadrec -i input.png output.png

    std::string algorithm = "deadrec";  // default value
    app.add_set("-a,--algorithm", algorithm, algoOptions());

    std::string imgPath;
    app.add_option("--image,-i", imgPath)->required()->check(CLI::ExistingFile);

    std::string outPath;
    app.add_option("outfile", outPath)->required();

    CLI11_PARSE(app, argc, argv);

    Image input = Image(imgPath);
    Image output = Image(input.getWidth(), input.getHeight(), sizeof(DistanceTransform::OutputType) * 8);
    std::unique_ptr<DistanceTransform> dt = dtFactory[algorithm](input, output);
    dt->transform();
    output.exportPng<DistanceTransform::OutputType>(outPath, 20, -30);
    return 0;
}

int main(int argc, char** argv) {
    llassetgen::init();

    CLI::App app{"OpenLL Font Asset Generator"};
    CLI::App* atlas = app.add_subcommand("atlas")->allow_extras();
    CLI::App* distfield = app.add_subcommand("distfield")->allow_extras();

    CLI11_PARSE(app, argc, argv);
    --argc;
    ++argv;

    if (app.got_subcommand(atlas)) {
        return parseAtlas(argc, argv);
    } else if (app.got_subcommand(distfield)) {
        return parseDistField(argc, argv);
    }
    return app.exit(CLI::CallForHelp());
}
