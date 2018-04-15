#include <CLI11.h>
#include <codecvt>
#include <map>
#include <ostream>

#include <algorithms.h>
#include <helpstrings.h>

#include <llassetgen/Atlas.h>
#include <llassetgen/FntWriter.h>
#include <llassetgen/FontFinder.h>

using namespace llassetgen;

// all printable ascii characters, except for space
constexpr char ascii[] =
    "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

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

std::vector<Vec2<size_t>> sizes(const std::vector<Image>& images, unsigned int downsamplingRatio) {
    std::vector<Vec2<size_t>> imageSizes(images.size());
    std::transform(images.begin(), images.end(), imageSizes.begin(),
                   [downsamplingRatio](const Image& img) { return img.getSize() / downsamplingRatio; });
    return imageSizes;
}

std::pair<std::string, std::string> outNames(const std::string& outPath) {
    std::string pathWithoutExtension;
    if (outPath.substr(outPath.length() - 4) == ".png") {
        pathWithoutExtension = outPath.substr(0, outPath.length() - 4);
    } else {
        pathWithoutExtension = outPath;
    }
    return std::make_pair(pathWithoutExtension + ".png", pathWithoutExtension + ".fnt");
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

    // positional arguments
    std::string outPath;
    app.add_option("outfile", outPath, aOutfileHelp)->required();

    // algorithms
    std::string algorithm;
    CLI::Option* distfieldOpt = app.add_set("-d, --distfield", algorithm, algoNames(dtAlgos), distfieldHelp);

    std::string packing = "shelf";
    app.add_set("-k, --packing", packing, algoNames(packingAlgos), packingHelp, true);

    // glyphs
    std::string glyphs;
    CLI::Option* glyphsOpt = app.add_option("-g, --glyph", glyphs, glyphHelp);

    std::vector<unsigned int> charCodes;
    app.add_option("-c, --charcode", charCodes, charcodeHelp);

    bool includeAscii = false;
    app.add_flag("--ascii", includeAscii, asciiHelp);

    // font
    unsigned int fontSize = 128;
    app.add_option("-s, --fontsize", fontSize, fontsizeHelp, true);

    std::string fontName;
    CLI::Option* fontNameOpt = app.add_option("-f, --fontname", fontName, fontnameHelp);

    std::string fontPath;
    CLI::Option* fontPathOpt = app.add_option("--fontpath", fontPath, fontpathHelp)->check(CLI::ExistingFile);

    // TODO: app.requires_one
    glyphsOpt->requires_one({fontPathOpt, fontNameOpt});

    // other options
    unsigned int padding = 0;
    app.add_option("-p, --padding", padding, paddingHelp);

    unsigned int downsamplingRatio = 1;
    app.add_option("-w, --downsampling", downsamplingRatio, downsamplingRatioHelp);

    std::string downsampling = "center";
    app.add_set("--dsalgo", downsampling, algoNames(downsamplingAlgos), downsamplingHelp, true);

    std::vector<int> dynamicRange = {-30, 20};
    app.add_option("-r, --dynamicrange", dynamicRange, dynamicrangeHelp, true)->requires(distfieldOpt)->expected(2);

    bool createFnt = false;
    app.add_flag("--fnt", createFnt, fntHelp);

    app.set_config("--config", "", configHelp);


    CLI11_PARSE(app, argc, argv);

    std::string fntPath;
    std::tie(outPath, fntPath) = outNames(outPath);

    std::set<unsigned long> glyphSet = makeGlyphSet(glyphs, charCodes, includeAscii);
    if (glyphSet.empty()) {
        std::cerr << "Error: at least one glyph required" << std::endl;
        return 2;
    }

    try {
        FontFinder fontFinder = fontPathOpt->count() ? FontFinder::fromPath(fontPath)
                                                     : FontFinder::fromName(fontName);

        std::vector<Image> glyphImages = fontFinder.renderGlyphs(glyphSet, fontSize, padding, downsamplingRatio);
        std::vector<Vec2<size_t>> imageSizes = sizes(glyphImages, downsamplingRatio);
        Packing p = packingAlgos[packing](imageSizes.begin(), imageSizes.end(), false);

        if (distfieldOpt->count()) {
            Image atlas = distanceFieldAtlas(glyphImages.begin(), glyphImages.end(), p, dtAlgos[algorithm], downsamplingAlgos[downsampling]);
            atlas.exportPng<DistanceTransform::OutputType>(outPath, - dynamicRange[0], - dynamicRange[1]);
        } else {
            Image atlas = fontAtlas(glyphImages.begin(), glyphImages.end(), p);
            atlas.exportPng<uint8_t>(outPath);
        }

        if (createFnt) {
            std::string faceName = fontNameOpt->count() ? fontName : "Unknown";
            FntWriter writer{fontFinder.fontFace, faceName, fontSize, 1, false};
            writer.setAtlasProperties(p.atlasSize, fontSize);
            writer.readFont(glyphSet.begin(), glyphSet.end());
            auto gIt = glyphSet.begin();
            for (auto rectIt = p.rects.begin(); rectIt < p.rects.end(); gIt++, rectIt++) {
                FT_UInt charIndex = FT_Get_Char_Index(fontFinder.fontFace, static_cast<FT_ULong>(*gIt));
                writer.setCharInfo(charIndex, *rectIt, {0, 0});
            }
            writer.saveFnt(fntPath);
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

    std::string algorithm = "parabola";
    app.add_set("-a, --algorithm", algorithm, algoNames(dtAlgos), algorithmHelp, true);

    std::string imgPath;
    app.add_option("image", imgPath, imageHelp)->required()->check(CLI::ExistingFile);

    std::string outPath;
    app.add_option("outfile", outPath, dOutfileHelp)->required();

    std::vector<int> dynamicRange = {-30, 20};
    app.add_option("-r, --dynamicrange", dynamicRange, dynamicrangeHelp, true)->expected(2);

    app.set_config("--config", "", configHelp);

    CLI11_PARSE(app, argc, argv);

    Image input = Image(imgPath);
    if (input.getBitDepth() != 1) {
        std::cerr << "Error: only black/white images are supported. Please use an image with a bit depth of 1."
                  << std::endl;
        return 2;
    }

    Image output = Image(input.getWidth(), input.getHeight(), DistanceTransform::bitDepth);
    dtAlgos[algorithm](input, output);
    output.exportPng<DistanceTransform::OutputType>(outPath, dynamicRange[1], dynamicRange[0]);
    return 0;
}

int main(int argc, char** argv) {
    llassetgen::init();

    CLI::App app{appHelp};
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
