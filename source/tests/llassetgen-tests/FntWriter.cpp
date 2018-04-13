#include <ft2build.h> // NOLINT include order required by freetype
#include FT_FREETYPE_H

#include <gmock/gmock.h>
#include <llassetgen/llassetgen.h>

#include <fstream>

using namespace llassetgen;

TEST(FntWriterTest, createFntFile) {
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
	float scaling_factor = 1.0f;
	FntWriter writer = FntWriter(face, face_name, font_size, scaling_factor, false);
	
	FT_UInt gIndex;
	std::set<FT_ULong> charcodes;
	FT_ULong last_charcode;
	last_charcode = FT_Get_First_Char(face, &gIndex);
	charcodes.insert(last_charcode);
	for (int i = 0; i < 50; i++) {
		last_charcode = FT_Get_Next_Char(face, last_charcode, &gIndex);
		charcodes.insert(last_charcode);
	}
	writer.readFont(charcodes.begin(), charcodes.end());

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

	writer.saveFnt(test_destination_path + "fnt.fnt");
}

TEST(FntWriterTest, fntScaling) {
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
	float scaling_factor = 0.5f;
	FntWriter writer = FntWriter(face, face_name, font_size, scaling_factor, true);
	
	FT_UInt gIndex;
	std::set<FT_ULong> charcodes;
	FT_ULong last_charcode;
	last_charcode = FT_Get_First_Char(face, &gIndex);
	charcodes.insert(last_charcode);
	for (int i = 0; i < 50; i++) {
		last_charcode = FT_Get_Next_Char(face, last_charcode, &gIndex);
		charcodes.insert(last_charcode);
	}
	writer.readFont(charcodes.begin(), charcodes.end());

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

	writer.saveFnt(test_destination_path + "fnt_scaled.fnt");
}