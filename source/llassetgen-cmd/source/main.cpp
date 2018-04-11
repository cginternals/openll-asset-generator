#include <CLI11.h>
#include <codecvt>
#include <map>
#include <ostream>

#include <FontFinder.h>
#include <helpstrings.h>
#include <llassetgen/Atlas.h>

using namespace llassetgen;

using VecIter = std::vector<Vec2<size_t>>::const_iterator;

std::map<std::string, void (*)(Image&, Image&)> dtAlgos{
    {"deadrec", [](Image& input, Image& output) { DeadReckoning(input, output).transform(); }},
    {"parabola", [](Image& input, Image& output) { ParabolaEnvelope(input, output).transform(); }},
};

std::map<std::string, Packing (*)(VecIter, VecIter, bool)> packingAlgos{
    {"shelf", shelfPackAtlas},
    {"maxrects", maxRectsPackAtlas}
};

// all printable ascii characters, except for space
constexpr char ascii[] =
    "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

template <class Func>
std::set<std::string> algoNames(std::map<std::string, Func> map) {
    std::set<std::string> options;
    for (const auto& algo : map) {
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
                   [padding](const Image& img) { return img.getSize(); });
    return imageSizes;
}

std::set<unsigned long> makeGlyphSet(const std::string& glyphs, const std::vector<unsigned int>& charCodes,
                                     bool includeAscii) {
    std::set<unsigned long> set;
    for (const auto c : UTF8toUCS4(glyphs)) {
        set.insert(c);
    }

    for (const auto c : charCodes) {
        set.insert(c);
    }

    if (includeAscii) {
        const char* p = ascii;
        while (*p) {
            set.insert(static_cast<unsigned long>(*p++));
        }
    }
    return set;
}

int parseAtlas(int argc, char** argv) {
    // Example: llassetgen-cmd atlas -d parabola --ascii -f Verdana atlas.png
    CLI::App app{atlasHelp};

    std::string algorithm;
    CLI::Option* distfieldOpt = app.add_set("-d, --distfield", algorithm, algoNames(dtAlgos), distfieldHelp);

    std::string packing = "shelf";
    app.add_set("-k, --packing", packing, algoNames(packingAlgos), packingHelp, true);

    std::string glyphs;
    CLI::Option* glyphsOpt = app.add_option("-g, --glyph", glyphs, glyphHelp);

    std::vector<unsigned int> charCodes;
    app.add_option("-c, --charcode", charCodes, charcodeHelp);

    std::string fontName;
    CLI::Option* fontNameOpt = app.add_option("-f, --fontname", fontName, fontnameHelp);

    std::string fontPath;
    CLI::Option* fontPathOpt = app.add_option("--fontpath", fontPath, fontpathHelp)->check(CLI::ExistingFile);

    // TODO: app.requires_one
    glyphsOpt->requires_one({fontPathOpt, fontNameOpt});

    int padding = 0;
    app.add_option("-p, --padding", padding, paddingHelp);

    int fontSize = 128;
    app.add_option("-s, --fontsize", fontSize, fontsizeHelp, true);

    std::vector<int> dynamicRange = {-30, 20};
    app.add_option("-r, --dynamicrange", dynamicRange, dynamicrangeHelp, true)->requires(distfieldOpt)->expected(2);

    CLI::Option* asciiOpt = app.add_flag("--ascii", asciiHelp);

    std::string outPath;
    app.add_option("outfile", outPath, aOutfileHelp)->required();

    CLI11_PARSE(app, argc, argv);

    std::set<unsigned long> glyphSet = makeGlyphSet(glyphs, charCodes, static_cast<bool>(asciiOpt->count()));
    if (glyphSet.empty()) {
        std::cerr << "Error: at least one glyph required" << std::endl;
        return 2;
    }

    try {
        FontFinder fontFinder = fontPathOpt->count() ? FontFinder::fromPath(fontPath)
                                                     : FontFinder::fromName(fontName);

        std::vector<Image> glyphImages = fontFinder.renderGlyphs(glyphSet, fontSize, padding);
        std::vector<Vec2<size_t>> imageSizes = sizes(glyphImages);
        Packing p = packingAlgos[packing](imageSizes.begin(), imageSizes.end(), false);

        if (distfieldOpt->count()) {
            Image atlas = distanceFieldAtlas(glyphImages.begin(), glyphImages.end(), p, dtAlgos[algorithm]);
            atlas.exportPng<DistanceTransform::OutputType>(outPath, dynamicRange[1], dynamicRange[0]);
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
    CLI::App app{dfHelp};
    // Example: llassetgen-cmd distfield -a deadrec -i input.png output.png

    std::string algorithm = "parabola";  // default value
    app.add_set("-a, --algorithm", algorithm, algoNames(dtAlgos), algorithmHelp, true);

    std::string imgPath;
    app.add_option("-i, --image", imgPath, imageHelp)->required()->check(CLI::ExistingFile);

    std::string outPath;
    app.add_option("outfile", outPath, dOutfileHelp)->required();

    std::vector<int> dynamicRange = {-30, 20};
    app.add_option("-r, --dynamicrange", dynamicRange, dynamicrangeHelp, true)->expected(2);

    CLI11_PARSE(app, argc, argv);

    Image input = Image(imgPath);
    Image output = Image(input.getWidth(), input.getHeight(), DistanceTransform::bitDepth);
    dtAlgos[algorithm](input, output);
    output.exportPng<DistanceTransform::OutputType>(outPath, dynamicRange[1], dynamicRange[0]);
    return 0;
}

int main(int argc, char** argv) {
    llassetgen::init();

    CLI::App app{"OpenLL Font Asset Generator"};
    // pseudo-subcommands to generate a help message, the actual parsing happens in the subcommand functions
    CLI::App* atlas = app.add_subcommand("atlas", atlasHelp)->allow_extras();
    CLI::App* distfield = app.add_subcommand("distfield", dfHelp)->allow_extras();

    atlas->set_help_flag();  // do not let the pseudo-subcommands parse the help flag, let the actual subcommands handle it
    distfield->set_help_flag();

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
