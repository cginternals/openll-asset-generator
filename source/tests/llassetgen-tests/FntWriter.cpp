#include <ft2build.h> // NOLINT include order required by freetype
#include FT_FREETYPE_H

#include <gmock/gmock.h>
#include <llassetgen/llassetgen.h>

#include <fstream>

using namespace llassetgen;

TEST(FntWriterTest, getKerning) {
	std::string test_source_path = "../../../source/tests/llassetgen-tests/testfiles/";
	std::string test_destination_path = "../../";

	init();

	FT_Face face;
	std::string face_name = "OpenSans-Regular.ttf";
	std::string font_file = test_source_path + face_name;

	FT_Error face_created = FT_New_Face(freetype, font_file.c_str(), 0, &face);
	EXPECT_EQ(face_created, 0);

	bool has_kerning = FT_HAS_KERNING(face);
	EXPECT_EQ(has_kerning, true);

	int font_size = 32;
	FntWriter writer = FntWriter(face, face_name, font_size);
	writer.readFont();

	FT_ULong charcode;
	FT_UInt gindex;
	Vec2<uint32_t> position = { 0, 0 };
	Vec2<uint32_t> size = { 1, 1 };
	Vec2<float> offset = { 0.0f, 0.0f };

	charcode = FT_Get_First_Char(face, &gindex);
	while (gindex != 0) {
		charcode = FT_Get_Next_Char(face, charcode, &gindex);
		writer.setCharInfo(gindex, Rect<uint32_t>(position, size), offset);
		position += {1, 1};
		size += {1, 1};
		offset += {1.0f, 1.0f};
	}

	int max_height = 20;
	writer.setAtlasProperties({ 100, 200 }, max_height);

	writer.SaveFnt(test_destination_path + "fnt.fnt");
}