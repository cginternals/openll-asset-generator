#include <iostream>
#include <map>

#include <CLI11.h>
#include <ft2build.h>  // NOLINT include order required by freetype
#include FT_FREETYPE_H

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

std::unique_ptr<Image> loadGlyph(FT_ULong glyph, std::string& font_path) {
    FT_Face face;
    assert(FT_New_Face(freetype, font_path.c_str(), 0, &face) == 0);
    assert(FT_Set_Pixel_Sizes(face, 0, 128) == 0);
    assert(FT_Load_Glyph(face, FT_Get_Char_Index(face, glyph), FT_LOAD_RENDER | FT_LOAD_TARGET_MONO) == 0);
    auto bitmap = std::shared_ptr<FT_Bitmap_>(new FT_Bitmap_(face->glyph->bitmap));
    return std::unique_ptr<Image>(new Image(*bitmap));
}

void distField(std::string& algorithm, Image& input, std::string& out_path) {
    Image output = Image(input.getWidth(), input.getHeight(), sizeof(DistanceTransform::OutputType) * 8);
    std::unique_ptr<DistanceTransform> dt = dtFactory[algorithm](input, output);
    dt->transform();
    output.exportPng<DistanceTransform::OutputType>(out_path, -20, 50);
}

std::u32string convertToUCS4(std::string& str) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> ucs4conv;
    return ucs4conv.from_bytes(str);
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
    CLI::Option* font_opt = distfield->add_option("--font,-f", font_path)->check(CLI::ExistingFile);
    font_opt->requires(glyph_opt);
    glyph_opt->requires(font_opt);

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
            // Example: llassetgen-cmd distfield -a parabola -g G --font="/Library/Fonts/Verdana.ttf" glyph.png
            std::u32string ucs4Glyph = convertToUCS4(glyph);
            if (ucs4Glyph.length() != 1) {
                std::cerr << "--glyph must be a single character" << std::endl;
                return 2;
            }
            input = loadGlyph(ucs4Glyph[0], font_path);
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
