#include <ft2build.h>  // NOLINT include order required by freetype
#include FT_FREETYPE_H

#include <gmock/gmock.h>
#include <llassetgen/llassetgen.h>

#include <fstream>

using namespace llassetgen;

TEST(FntWriterTest, createFntFile) {
	std::string testSourcePath = "../../../source/tests/llassetgen-tests/testfiles/";
	std::string testDestinationPath = "../../";

	init();

	FT_Face face;
	std::string faceName = "OpenSans-Regular.ttf";
	std::string fontFile = testSourcePath + faceName;

	FT_Error faceCreated = FT_New_Face(freetype, fontFile.c_str(), 0, &face);
	EXPECT_EQ(faceCreated, 0);

	bool hasKerning = FT_HAS_KERNING(face);
	EXPECT_EQ(hasKerning, true);

	unsigned int fontSize = 32;
	float scalingFactor = 1.0f;
	float padding = 0;
	FntWriter writer = FntWriter(face, faceName, fontSize, scalingFactor, padding);
	
	FT_UInt gIndex;
	std::set<FT_ULong> charcodes;
	FT_ULong lastCharcode;
	lastCharcode = FT_Get_First_Char(face, &gIndex);
	charcodes.insert(lastCharcode);
	for (int i = 0; i < 50; i++) {
		lastCharcode = FT_Get_Next_Char(face, lastCharcode, &gIndex);
		charcodes.insert(lastCharcode);
	}
	writer.readFont(charcodes.begin(), charcodes.end());

	FT_ULong charcode;
	FT_UInt gindex;
	Vec2<PackingSizeType> position = { 0, 0 };
	Vec2<PackingSizeType> size = { 1, 1 };
	Vec2<float> offset = { 0.0f, 0.0f };

	charcode = FT_Get_First_Char(face, &gindex);
	while (gindex != 0) {
		charcode = FT_Get_Next_Char(face, charcode, &gindex);
		writer.setCharInfo(gindex, Rect<PackingSizeType>(position, size));
		position += {1, 1};
		size += {1, 1};
		offset += {1.0f, 1.0f};
	}

	//int maxHeight = 20;
	writer.setAtlasProperties({ 100, 200 });

	writer.saveFnt(testDestinationPath + "fnt.fnt");
}

TEST(FntWriterTest, fntScaling) {
	std::string testSourcePath = "../../../source/tests/llassetgen-tests/testfiles/";
	std::string testDestinationPath = "../../";

	init();

	FT_Face face;
	std::string faceName = "OpenSans-Regular.ttf";
	std::string fontFile = testSourcePath + faceName;

	FT_Error faceCreated = FT_New_Face(freetype, fontFile.c_str(), 0, &face);
	EXPECT_EQ(faceCreated, 0);

	bool hasKerning = FT_HAS_KERNING(face);
	EXPECT_EQ(hasKerning, true);

	unsigned int fontSize = 32;
	float scalingFactor = 0.5f;
	float padding = 0;
	FntWriter writer = FntWriter(face, faceName, fontSize, scalingFactor, padding);
	
	FT_UInt gIndex;
	std::set<FT_ULong> charcodes;
	FT_ULong lastCharcode;
	lastCharcode = FT_Get_First_Char(face, &gIndex);
	charcodes.insert(lastCharcode);
	for (int i = 0; i < 50; i++) {
		lastCharcode = FT_Get_Next_Char(face, lastCharcode, &gIndex);
		charcodes.insert(lastCharcode);
	}
	writer.readFont(charcodes.begin(), charcodes.end());

	FT_ULong charcode;
	FT_UInt gindex;
	Vec2<PackingSizeType> position = { 0, 0 };
	Vec2<PackingSizeType> size = { 1, 1 };
	Vec2<float> offset = { 0.0f, 0.0f };

	charcode = FT_Get_First_Char(face, &gindex);
	while (gindex != 0) {
		charcode = FT_Get_Next_Char(face, charcode, &gindex);
		writer.setCharInfo(gindex, Rect<PackingSizeType>(position, size));
		position += {1, 1};
		size += {1, 1};
		offset += {1.0f, 1.0f};
	}

	//int maxHeight = 20;
	writer.setAtlasProperties({ 100, 200 });

	writer.saveFnt(testDestinationPath + "fnt_scaled.fnt");
}
