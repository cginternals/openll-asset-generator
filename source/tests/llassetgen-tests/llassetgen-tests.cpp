#include <ft2build.h> // NOLINT include order required by freetype
#include FT_FREETYPE_H

#include <gmock/gmock.h>
#include <llassetgen/llassetgen.h>

#include <fstream>

using namespace llassetgen;

class llassetgen_tests : public testing::Test {

};

/*
TEST_F(llassetgen_tests, DeadReckoning) {
    Image input("input.png"), output(input.get_width(), input.get_height(), sizeof(DistanceTransform::OutputType)*8);
    std::unique_ptr<DistanceTransform> dt(new DeadReckoning(input, output));
    dt->transform();
    output.exportPng<DistanceTransform::OutputType>("DeadReckoning.png", -20, 50);
    EXPECT_EQ(1, 1);
}

TEST_F(llassetgen_tests, ParabolaEnvelope) {
    Image input("input.png"), output(input.get_width(), input.get_height(), sizeof(DistanceTransform::OutputType)*8);
    std::unique_ptr<DistanceTransform> dt(new ParabolaEnvelope(input, output));
    dt->transform();
    output.exportPng<DistanceTransform::OutputType>("ParabolaEnvelope.png", -20, 50);
    EXPECT_EQ(1, 1);
}*/

//std::string testfile_path = "./";
std::string testfile_path = "./source/tests/llassetgen-tests/testfiles/";

TEST(ImageTest, LoadTTF) {
    init();

    FT_Face face;
    std::string font_file = testfile_path + "arial.ttf";
    char letter = 'J';

    FT_Error face_created = FT_New_Face(freetype, font_file.c_str(), 0, &face);
    EXPECT_EQ(face_created, 0);

    FT_Error pixel_size_set = FT_Set_Pixel_Sizes(face, 64, 64);
    EXPECT_EQ(pixel_size_set, 0);

    FT_Error glyph_loaded = FT_Load_Glyph(face, FT_Get_Char_Index(face, letter), FT_LOAD_RENDER | FT_LOAD_MONOCHROME); // load default 8-bit //| FT_LOAD_MONOCHROME for 1-bit
    EXPECT_EQ(glyph_loaded, 0);

    Image ft_8bit(face->glyph->bitmap);

    // view on (10,10),(20,20)
    Image view_on_ft_8bit(ft_8bit.view(Vec2<size_t>(10, 10), Vec2<size_t>(20, 20)));
    // invert view
    for (size_t y = 0; y < view_on_ft_8bit.get_height(); y++) {
        for (size_t x = 0; x < view_on_ft_8bit.get_width(); x++) {
            view_on_ft_8bit.setPixel<uint8_t>(Vec2<size_t>(x, y), 1 - view_on_ft_8bit.getPixel<uint8_t>(Vec2<size_t>(x, y)));
        }
    }

    // save whole image.
    ft_8bit.exportPng<uint8_t>(testfile_path + "glyph1_out.png");
}

TEST(ImageTest, CreateAndWriteOneBitPNG) {
    Image blank_1bit(2, 2, 1);
    blank_1bit.setPixel<uint8_t>(Vec2<size_t>(0, 0), 1);
    blank_1bit.setPixel<uint8_t>(Vec2<size_t>(1, 1), 1);
    blank_1bit.exportPng<uint8_t>(testfile_path + "blank_one_bit.png");
}

TEST(ImageTest, CreateAndWritePNG) {
    // create blank image with 40x30 pixels and 16 bit
    Image blank_16bit(8, 8, 16);
    // set pixels
    blank_16bit.setPixel<uint16_t>(Vec2<size_t>(3, 4), 26781);
    blank_16bit.setPixel<uint16_t>(Vec2<size_t>(4, 5), 42949);

    // get pixels and test
    for (size_t y = 0; y < blank_16bit.get_height(); y++) {
        for (size_t x = 0; x < blank_16bit.get_width(); x++) {
            if (x == 3 && y == 4) {
                EXPECT_EQ(blank_16bit.getPixel<uint32_t>(Vec2<size_t>(x, y)), 26781);
            }
            else if (x == 4 && y == 5) {
                EXPECT_EQ(blank_16bit.getPixel<uint32_t>(Vec2<size_t>(x, y)), 42949);
            }
            else {
                EXPECT_EQ(blank_16bit.getPixel<uint32_t>(Vec2<size_t>(x, y)), 0);
            }
        }
    }
    blank_16bit.exportPng<uint16_t>(testfile_path + "blank.png");
}

TEST(ImageTest, LoadPNGwithView) {
    Image loaded_png(testfile_path + "A_glyph.png");
    // view on (10,10),(20,20)
    Image view_on_loaded_png(loaded_png.view(Vec2<size_t>(10, 10), Vec2<size_t>(20, 20)));
    // invert view
    for (size_t y = 0; y < view_on_loaded_png.get_height(); y++) {
        for (size_t x = 0; x < view_on_loaded_png.get_width(); x++) {
            view_on_loaded_png.setPixel<uint16_t>(Vec2<size_t>(x, y), 0xFF - view_on_loaded_png.getPixel<uint16_t>(Vec2<size_t>(x, y)));
        }
    }
    loaded_png.exportPng<uint16_t>(testfile_path + "A_glyph2_out.png");
}

TEST(ImageTest, LoadPNGReducedBitDepth) {
    Image loaded_png4(testfile_path + "A_glyph.png", 4);
    loaded_png4.exportPng<uint16_t>(testfile_path + "A_glyph2_out_4bit.png");
    Image loaded_png2(testfile_path + "A_glyph.png", 2);
    loaded_png2.exportPng<uint16_t>(testfile_path + "A_glyph2_out_2bit.png");
    Image loaded_png1(testfile_path + "A_glyph.png", 1);
    loaded_png1.exportPng<uint16_t>(testfile_path + "A_glyph2_out_1bit.png");
}

TEST(ImageTest, FloatExport) {
    Image float_image(256, 256, 32);
    for (size_t y = 0; y < float_image.get_height(); y++) {
        for (size_t x = 0; x < float_image.get_width(); x++) {
            float_image.setPixel<float>(Vec2<size_t>(x, y), float(x + y));
        }
    }

    float_image.exportPng<float>(testfile_path + "float.png", 0.0f, float(float_image.get_width() + float_image.get_height()));
}
