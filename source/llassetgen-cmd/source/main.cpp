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

constexpr char16_t preset20180319[] = {
    u'\x01',   u'\x03',   u'\x07',   u'\x11',   u' ',      u'!',      u'#',      u'$',      u'%',      u'&',
    u'\x27',   u'(',      u')',      u'*',      u'+',      u',',      u'-',      u'.',      u'/',      u'0',
    u'1',      u'2',      u'3',      u'4',      u'5',      u'6',      u'7',      u'8',      u'9',      u':',
    u';',      u'<',      u'=',      u'>',      u'?',      u'@',      u'A',      u'B',      u'C',      u'D',
    u'E',      u'F',      u'G',      u'H',      u'I',      u'J',      u'K',      u'L',      u'M',      u'N',
    u'O',      u'P',      u'Q',      u'R',      u'S',      u'T',      u'U',      u'V',      u'W',      u'X',
    u'Y',      u'Z',      u'[',      u'\\',     u']',      u'^',      u'_',      u'`',      u'a',      u'b',
    u'c',      u'd',      u'e',      u'f',      u'g',      u'h',      u'i',      u'j',      u'k',      u'l',
    u'm',      u'n',      u'o',      u'p',      u'q',      u'r',      u's',      u't',      u'u',      u'v',
    u'w',      u'x',      u'y',      u'z',      u'{',      u'}',      u'~',      u'\x7f',   u'\x84',   u'\x9c',
    u'\xa4',   u'\xb0',   u'\xb6',   u'\xbc',   u'\xc3',   u'\xc4',   u'\xd3',   u'\xd6',   u'\xdc',   u'\xdf',
    u'\xe0',   u'\xe1',   u'\xe4',   u'\xf0',   u'\xf3',   u'\xf6',   u'\xfc',   u'\u0178', u'\u0394', u'\u03ae',
    u'\u03b1', u'\u03b9', u'\u03bc', u'\u03bf', u'\u03c1', u'\u03c4', u'\u03c6', u'\u03c9', u'\u041a', u'\u0430',
    u'\u0433', u'\u0438', u'\u043d', u'\u043e', u'\u0440', u'\u0442', u'\u0443', u'\u0444', u'\u2013', u'\u20ac',
    u'\u2192', u'\u3001', u'\u30a3', u'\u30ae', u'\u30b3', u'\u30bf', u'\u30d5', u'\u30e5', u'\u30ec', u'\u30f3',
    u'\u30fc', u'\u4e09', u'\u4e2a', u'\u4e2d', u'\u4e3a', u'\u4e3b', u'\u4e49', u'\u4e92', u'\u4ea4', u'\u4ea7',
    u'\u4eba', u'\u4ecb', u'\u4ed6', u'\u4ee3', u'\u4ef6', u'\u4fdd', u'\u4fe1', u'\u503a', u'\u505a', u'\u516c',
    u'\u5176', u'\u5177', u'\u51ed', u'\u5230', u'\u5238', u'\u526f', u'\u52a0', u'\u5305', u'\u5316', u'\u5355',
    u'\u5386', u'\u539f', u'\u53d1', u'\u53f0', u'\u53f2', u'\u54c1', u'\u552e', u'\u56e0', u'\u573a', u'\u578b',
    u'\u5904', u'\u5916', u'\u5931', u'\u5b57', u'\u5b9a', u'\u5b9e', u'\u5de5', u'\u5df2', u'\u5e02', u'\u5e73',
    u'\u5f00', u'\u5f52', u'\u5f55', u'\u6027', u'\u606f', u'\u627f', u'\u6295', u'\u62bc', u'\u62cd', u'\u6362',
    u'\u636e', u'\u63d0', u'\u6444', u'\u6536', u'\u6570', u'\u6587', u'\u65b0', u'\u65e5', u'\u6613', u'\u672c',
    u'\u677f', u'\u6784', u'\u679c', u'\u67e5', u'\u6848', u'\u6863', u'\u6b3e', u'\u6bb5', u'\u6d4b', u'\u6d88',
    u'\u6dfb', u'\u6e20', u'\u6e70', u'\u6e90', u'\u7406', u'\u7528', u'\u7533', u'\u754c', u'\u767b', u'\u7684',
    u'\u76ee', u'\u7801', u'\u7968', u'\u7a7a', u'\u7acb', u'\u7ed3', u'\u80a1', u'\u8272', u'\u8425', u'\u884c',
    u'\u8868', u'\u88fd', u'\u8907', u'\u89c8', u'\u89d2', u'\u8bc1', u'\u8bd5', u'\u8be2', u'\u8bf7', u'\u8c03',
    u'\u8d25', u'\u8d26', u'\u8d28', u'\u8d37', u'\u8d39', u'\u8f6c', u'\u8f91', u'\u903b', u'\u9053', u'\u9353',
    u'\u94f6', u'\u9500', u'\u9636', u'\u9644', u'\u9762', u'\u9875', u'\u9879', u'\u9884', u'\ue21b', u'\uff0d',
    u'\uff0f', u'\ufffd'};

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
                                     const std::string& presetName) {
    std::set<unsigned long> set;
    for (const auto c : UTF8toUCS4(glyphs)) {
        set.insert(c);
    }

    for (const auto c : charCodes) {
        set.insert(c);
    }

    if (presetName == "ascii") {
        const char* p = ascii;
        while (*p) {
            set.insert(static_cast<unsigned long>(*p++));
        }
    } else if (presetName == "preset20180319") {
        const char16_t* p = preset20180319;
        while (*p) {
            set.insert(static_cast<unsigned long>(*p++));
        }
    } else {
        std::cerr << "Error: No preset found for given preset name." << std::endl;
    }
    return set;
}

void checkIfFontSet(CLI::Option* nameOpt, CLI::Option* pathOpt) {
    if (!static_cast<bool>(*nameOpt) && !static_cast<bool>(*pathOpt)) {
        throw std::runtime_error("no font specified");
    }
}

int parseAtlasArgs(int argc, char** argv) {
    // Example: llassetgen-cmd atlas -d parabola --preset preset20180319 -f Verdana atlas.png
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
    app.add_option("-g, --glyph", glyphs, glyphHelp);

    std::string presetName;
    /*CLI::Option* presetNameOpt = */ app.add_option("--preset", presetName, presetHelp);

    std::vector<unsigned int> charCodes;
    app.add_option("-c, --charcode", charCodes, charcodeHelp);

    // font
    unsigned int fontSize = 128;
    app.add_option("-s, --fontsize", fontSize, fontsizeHelp, true);

    std::string fontName;
    CLI::Option* fontNameOpt = app.add_option("-f, --fontname", fontName, fontnameHelp);

    std::string fontPath;
    CLI::Option* fontPathOpt = app.add_option("--fontpath", fontPath, fontpathHelp)->check(CLI::ExistingFile);

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

    std::set<unsigned long> glyphSet = makeGlyphSet(glyphs, charCodes, presetName);
    if (glyphSet.empty()) {
        std::cerr << "Error: at least one glyph required" << std::endl;
        return 2;
    }

    try {
        checkIfFontSet(fontNameOpt, fontPathOpt);
        FontFinder fontFinder =
            static_cast<bool>(*fontPathOpt) ? FontFinder::fromPath(fontPath) : FontFinder::fromName(fontName);

        std::vector<Image> glyphImages = fontFinder.renderGlyphs(glyphSet, fontSize, padding, downsamplingRatio);
        std::vector<Vec2<size_t>> imageSizes = sizes(glyphImages, downsamplingRatio);
        Packing p = packingAlgos[packing](imageSizes.begin(), imageSizes.end(), false);

        if (static_cast<bool>(*distfieldOpt)) {
            Image atlas = distanceFieldAtlas(glyphImages.begin(), glyphImages.end(), p, dtAlgos[algorithm],
                                             downsamplingAlgos[downsampling]);
            atlas.exportPng<DistanceTransform::OutputType>(outPath, -dynamicRange[0], -dynamicRange[1]);
        } else {
            Image atlas = fontAtlas(glyphImages.begin(), glyphImages.end(), p);
            atlas.exportPng<uint8_t>(outPath);
        }

        if (createFnt) {
            std::string faceName = static_cast<bool>(*fontNameOpt) ? fontName : "Unknown";
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

int parseDistfieldArgs(int argc, char** argv) {
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

    atlas->set_help_flag();  // do not let the pseudo-subcommands parse the help flag
                             // let the actual subcommands handle it
    distfield->set_help_flag();

    CLI11_PARSE(app, argc, argv);
    --argc;
    ++argv;

    if (app.got_subcommand(atlas)) {
        return parseAtlasArgs(argc, argv);
    } else if (app.got_subcommand(distfield)) {
        return parseDistfieldArgs(argc, argv);
    }
    return app.exit(CLI::CallForHelp());
}
