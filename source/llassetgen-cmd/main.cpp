#include <iostream>

#include <CLI11.h>
#include <ft2build.h> // NOLINT include order required by freetype
#include FT_FREETYPE_H

#include <llassetgen/llassetgen.h>

using namespace llassetgen;

DistanceTransform *instantiate(std::string algorithm) {
    if(algorithm == "dreck") return new DeadReckoning();
    if(algorithm == "parabola") return new ParabolaEnvelope();
    assert(false);
}

void dtFromGlyph(std::string algorithm, FT_ULong glyph, std::string font_path, std::string out_path) {
    std::unique_ptr<DistanceTransform> dt(instantiate(algorithm));
    FT_Face face;
    assert(FT_New_Face(freetype, font_path.c_str(), 0, &face) == 0);
    assert(FT_Set_Pixel_Sizes(face, 0, 128) == 0);
    assert(FT_Load_Glyph(face, FT_Get_Char_Index(face, glyph), FT_LOAD_RENDER | FT_LOAD_TARGET_MONO) == 0);
    dt->importFreeTypeBitmap(&face->glyph->bitmap, 20);
    dt->transform();
    dt->exportPng(out_path, -10, 20, 8);
}

void dtFromPng(std::string algorithm, std::string png_path, std::string out_path) {
    std::unique_ptr<DistanceTransform> dt(instantiate(algorithm));
    dt->importPng(png_path);
    dt->transform();
    dt->exportPng(out_path, -20, 50, 8);
}

int main(int argc, char** argv) {
    llassetgen::init();

    CLI::App app{"OpenLL Font Asset Generator"};
    CLI::App *distfield = app.add_subcommand("distfield");

    std::string algorithm = "dreck";  // default value
    std::set<std::string> algo_options {"dreck", "parabola"};
    distfield->add_set("-a,--algorithm", algorithm, algo_options);

    std::string glyph;
    CLI::Option *glyph_opt = distfield->add_option("-g,--glyph", glyph);

    std::string font_path;
    CLI::Option * font_opt = distfield->add_option("--font,-f", font_path)->check(CLI::ExistingFile);
    font_opt->requires(glyph_opt);
    glyph_opt->requires(font_opt);

    std::string png_path;
    distfield->add_option("--image,-i", png_path)->excludes(glyph_opt);

    std::string out_path;
    distfield->add_option("output", out_path)->required();

    CLI11_PARSE(app, argc, argv);

    if(app.got_subcommand(distfield)) {
        if (distfield->count("-g")) {
            // Example: llassetgen-cmd distfield -a parabola -g G --font="/Library/Fonts/Verdana.ttf" glyph.png
            if (glyph.length() != 1) {  // TODO convert from std::string to FT_ULong to allow non-ASCII
                std::cerr << "--glyph must be a single character" << std::endl;
                exit(2);
            }
            dtFromGlyph(algorithm, glyph[0], font_path, out_path);
        } else if (distfield->count("-i")) {
            // Example: llassetgen-cmd distfield -a dreck -i input.png output.png
            dtFromPng(algorithm, png_path, out_path);
        }
    }
    return 0;
}
