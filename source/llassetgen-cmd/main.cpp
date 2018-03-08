#include <iostream>
#include <map>

#include <CLI11.h>
#include <ft2build.h>  // NOLINT include order required by freetype
#include FT_FREETYPE_H

#ifdef __unix__
#include <fontconfig/fontconfig.h>
#elif _WIN32
// TODO
#endif

#include <llassetgen/llassetgen.h>
#include <codecvt>

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

std::unique_ptr<Image> renderGlyph(FT_ULong glyph, const FT_Face& face) {
    assert(FT_Set_Pixel_Sizes(face, 0, 128) == 0);
    assert(FT_Load_Glyph(face, FT_Get_Char_Index(face, glyph), FT_LOAD_RENDER | FT_LOAD_TARGET_MONO) == 0);
    auto bitmap = std::shared_ptr<FT_Bitmap_>(new FT_Bitmap_(face->glyph->bitmap));
    return std::unique_ptr<Image>(new Image(*bitmap));
}

std::u32string convertToUCS4(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> ucs4conv;
    return ucs4conv.from_bytes(str);
}

std::unique_ptr<Image> loadGlyphFromPath(const std::string& glyph, const std::string& font_path) {
    std::u32string ucs4Glyph = convertToUCS4(glyph);
    if (ucs4Glyph.length() != 1) {
        std::cerr << "--glyph must be a single character" << std::endl;
        return nullptr;
    }

    FT_Face face;
    assert(FT_New_Face(freetype, font_path.c_str(), 0, &face) == 0);
    return renderGlyph(ucs4Glyph[0], face);
}

#ifdef __linux__
bool findFontPath(const std::string& fontName, std::string& fontPath) {
    FcConfig* config = FcInitLoadConfigAndFonts();
    FcPattern* pat = FcNameParse(reinterpret_cast<const FcChar8*>(fontName.c_str()));
    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    FcResult result;
    FcPattern* font = FcFontMatch(config, pat, &result);

    bool found = false;
    if (result == FcResultMatch) {
        FcChar8* file;
        found = FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch;
        if (found) {
            fontPath.assign(reinterpret_cast<char*>(file));
        }
        FcPatternDestroy(font);
    }
    FcPatternDestroy(pat);
    return found;
}

std::unique_ptr<Image> loadGlyphFromName(const std::string& glyph, const std::string& font_name) {
    std::string fontPath;
    if (!findFontPath(font_name, fontPath)) {
        std::cerr << "Font not found" << std::endl;
        return nullptr;
    }
    return loadGlyphFromPath(glyph, fontPath);
}
#elif _WIN32
std::unique_ptr<Image> loadGlyphFromName(const std::string& glyph, const std::string& font_name) {
    std::u32string ucs4Glyph = convertToUCS4(glyph);
    if (ucs4Glyph.length() != 1) {
        std::cerr << "--glyph must be a single character" << std::endl;
        return nullptr;
    }

// TODO get font data
    FT_Long dataLen;
    FTByte fontData[dataLen];
    FT_Face face;
    FT_New_Memory_Face(freetype, fontData, dataLen, 0, &face);
    return renderGlyph(ucs4Glyph[0], face);
}
#endif

void distField(std::string& algorithm, Image& input, std::string& out_path) {
    Image output = Image(input.getWidth(), input.getHeight(), sizeof(DistanceTransform::OutputType) * 8);
    std::unique_ptr<DistanceTransform> dt = dtFactory[algorithm](input, output);
    dt->transform();
    output.exportPng<DistanceTransform::OutputType>(out_path, -20, 50);
}

int main(int argc, char** argv) {
    llassetgen::init();

    CLI::App app{"OpenLL Font Asset Generator"};
    CLI::App* distfield = app.add_subcommand("distfield");
    CLI::App* atlas = app.add_subcommand("atlas");

    // distfield params
    std::string algorithm = "deadrec";  // default value
    std::set<std::string> algo_options;
    for (auto& algo : dtFactory) {
        algo_options.insert(algo.first);
    }
    distfield->add_set("-a,--algorithm", algorithm, algo_options);

    std::string glyph;
    CLI::Option* glyph_opt = distfield->add_option("-g,--glyph", glyph);

    std::string font_path;
    CLI::Option* font_path_opt = distfield->add_option("--fontpath", font_path)->check(CLI::ExistingFile);
    font_path_opt->requires(glyph_opt);

    std::string font_name;
    CLI::Option* font_name_opt = distfield->add_option("--fontname,-f", font_name);
    font_name_opt->requires(glyph_opt);

    glyph_opt->requires_one({font_path_opt, font_name_opt});

    std::string img_path;
    CLI::Option* img_opt = distfield->add_option("--image,-i", img_path)->check(CLI::ExistingFile);
    img_opt->excludes(glyph_opt);

    std::string out_path;
    distfield->add_option("outfile", out_path)->required();

    // atlas params
    // TODO

    CLI11_PARSE(app, argc, argv);

    if (app.got_subcommand(distfield)) {
        std::unique_ptr<Image> input;
        if (glyph_opt->count()) {
            // Example: llassetgen-cmd distfield -a parabola -g G -f Verdana glyph.png
            input = font_path_opt->count() ? loadGlyphFromPath(glyph, font_path)
                                           : loadGlyphFromName(glyph, font_name);
        } else if (img_opt->count()) {
            // Example: llassetgen-cmd distfield -a deadrec -i input.png output.png
            input = std::unique_ptr<Image>(new Image(img_path));  // TODO error handling
        } else {
            return distfield->exit(CLI::CallForHelp());
        }

        distField(algorithm, *input, out_path);
    } else if (app.got_subcommand(atlas)) {
        // TODO
    } else {
        return app.exit(CLI::CallForHelp());
    }

    return 0;
}
