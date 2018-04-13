#include <fstream> 
#include <float.h>
#include <string>
#include <vector>
#include <cassert>
#include <iostream>
#include <set>

#include <llassetgen/FntWriter.h>
#include <llassetgen/Image.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

namespace llassetgen {
	FntWriter::FntWriter(FT_Face _face, std::string _faceName, unsigned int _fontSize, float _scalingFactor, bool _scaledGlyph) {
		faceName = _faceName;
		face = _face;
		fontInfo = Info();
		fontCommon = Common();
		charInfos = std::vector<CharInfo>();
		kerningInfos = std::vector<KerningInfo>();
		fontInfo.size = _fontSize;
		maxYBearing = 0;
		scalingFactor = _scalingFactor;
		scaledGlyph = _scaledGlyph;
	}

	void FntWriter::setFontInfo() {
		// collect fontInfo 
		fontInfo.face = face->family_name + std::string(" ") + face->style_name;
		fontInfo.isBold = false;
		fontInfo.isItalic = false;
		if (face->style_flags == FT_STYLE_FLAG_ITALIC) {
			fontInfo.isItalic = true;
		}
		if (face->style_flags == FT_STYLE_FLAG_BOLD) {
			fontInfo.isBold = true;
		}
		int encoding = int(face->charmap->encoding);
		uint8_t *character = reinterpret_cast<uint8_t*>(&encoding);
		for (int i = 3; i >= 0; --i) {
			fontInfo.charset += char(character[i]);
		}

		fontInfo.useUnicode = (face->charmap->encoding == FT_ENCODING_UNICODE);

		// havent found any of the following information
		// irrelevant for distance fields --> ignore them
		/*
		fontInfo.stretchH = 1.0f;
		fontInfo.useSmoothing = false;
		fontInfo.supersamplingLevel = 1;
		fontInfo.padding = { 2.8f, 2.8f, 2.8f, 2.8f };
		fontInfo.spacing = { 0, 0 };
		fontInfo.outlineThickness = 0;
		*/
	}

	void FntWriter::setCharInfo(FT_UInt gindex, Rect<PackingSizeType> charArea, Vec2<float> offset) {
		FT_Load_Glyph(face,	gindex, FT_LOAD_DEFAULT);
		
		FT_Pos yBearing = face->glyph->metrics.vertBearingY;
		maxYBearing = std::max(yBearing, maxYBearing);

		CharInfo charInfo;
		charInfo.id = gindex;
		charInfo.x = charArea.position.x;
		charInfo.y = charArea.position.y;
		charInfo.width = charArea.size.x;
		charInfo.height = charArea.size.y;
		charInfo.xAdvance = float(face->glyph->linearHoriAdvance) / 65536.f;
		charInfo.xOffset = offset.x;
		charInfo.yOffset = offset.y;
		charInfo.page = 1;
		charInfo.chnl = 15;
		charInfos.push_back(charInfo);
	}

	void FntWriter::setKerningInfo(std::set<FT_ULong>::iterator charcodesBegin, std::set<FT_ULong>::iterator charcodesEnd) {
		std::set<FT_ULong>::iterator leftCharcode = charcodesBegin;
		for (std::set<FT_ULong>::iterator leftCharcode = charcodesBegin; leftCharcode != charcodesEnd; leftCharcode++) {
			for (std::set<FT_ULong>::iterator rightCharcode = charcodesBegin; rightCharcode != charcodesEnd; rightCharcode++) {
				FT_UInt leftGindex = FT_Get_Char_Index(face, *leftCharcode);
				FT_UInt rightGindex = FT_Get_Char_Index(face, *rightCharcode);
				FT_Vector kerningVector;
				FT_Get_Kerning(face, leftGindex, rightGindex, FT_KERNING_UNSCALED, &kerningVector);
				if (kerningVector.x != 0) {
					KerningInfo kerningInfo;
					kerningInfo.firstId = leftGindex;
					kerningInfo.secondId = rightGindex;
					// kerning is provided in 26.6 fixed-point format
					kerningInfo.kerning = float(kerningVector.x) / 64.f;
					kerningInfos.push_back(kerningInfo);
				}
			}
		}
	}

	void FntWriter::readFont(std::set<FT_ULong>::iterator charcodesBegin, std::set<FT_ULong>::iterator charcodesEnd) {
		setFontInfo();
		setKerningInfo(charcodesBegin, charcodesEnd);
	}

	void FntWriter::setAtlasProperties(Vec2<PackingSizeType> size, int maxHeight) {
		// collect commonInfo
		fontCommon.lineHeight = maxHeight;
		fontCommon.scaleW = size.x;
		fontCommon.scaleH = size.y;
		fontCommon.pages = 1;
		fontCommon.isPacked = 0;
	}
	
	void FntWriter::saveFnt(std::string filepath) {
		fontCommon.base = maxYBearing;

		// open file
		std::ofstream fntFile;
		fntFile.open(filepath);

		// write in plain text format
		// TODO: XML and binary

		// write info block
		fntFile << "info "
			<< "face=\"" << fontInfo.face << "\" "
			<< "size=" << fontInfo.size << " "
			<< "bold=" << int(fontInfo.isBold) << " "
			<< "italic=" << int(fontInfo.isItalic) << " "
			<< "charset=\"" << fontInfo.charset << "\" "
			<< "unicode=" << int(fontInfo.useUnicode) 
			/* << " "
			<< "stretchH=" << fontInfo.stretchH << " "
			<< "smooth=" << int(fontInfo.useSmoothing) << " "
			<< "aa=" << fontInfo.supersamplingLevel << " "
			<< "padding=" << fontInfo.padding.up << "," << fontInfo.padding.right << "," << fontInfo.padding.down << "," << fontInfo.padding.left << " "
			<< "spacing=" << fontInfo.spacing.horiz << "," << fontInfo.spacing.vert << " "
			<< "outline=" << fontInfo.outlineThickness
			*/
			<< std::endl;

		// write common block
		fntFile << "common "
			<< "lineHeight=" << float(fontCommon.lineHeight) * scalingFactor << " "
			<< "base=" << float(fontCommon.base) * scalingFactor << " "
			<< "scaleW=" << (scaledGlyph ? (float(fontCommon.scaleW) * scalingFactor) : fontCommon.scaleW) << " "
			<< "scaleH=" << (scaledGlyph ? (float(fontCommon.scaleH) * scalingFactor) : fontCommon.scaleH) << " "
			<< "pages=" << fontCommon.pages << " "
			<< "packed=" << int(fontCommon.isPacked) << std::endl;

		// write page files
		for (int i = 0; i < fontCommon.pages; i++) {
			fntFile << "page "
				<< "id=" << i << " "
				<< "file=\"" << faceName << "\"" << std::endl;
		}

		// write char count
		fntFile << "chars count=" << charInfos.size() << std::endl;

		// write info for each char
		for (auto charInfo : charInfos) {
			fntFile << "char "
				<< "id=" << charInfo.id << " "
				<< "x=" << charInfo.x << " "
				<< "y=" << charInfo.y << " "
				<< "width=" << (scaledGlyph ? (float(charInfo.width) * scalingFactor) : charInfo.width) << " "
				<< "height=" << (scaledGlyph ? (float(charInfo.height) * scalingFactor) : charInfo.height) << " "
				<< "xoffset=" << charInfo.xOffset << " "
				<< "yoffset=" << charInfo.yOffset << " "
				<< "xadvance=" << float(charInfo.xAdvance) * scalingFactor << " "
				<< "page=" << charInfo.page << " "
				<< "chnl=" << int(charInfo.chnl) << std::endl;
		}

		// write kerning count
		fntFile << "kernings count=" << kerningInfos.size() << std::endl;

		// write each kerning info
		for (auto kerningInfo : kerningInfos) {
			fntFile << "kerning "
				<< "first=" << kerningInfo.firstId << " "
				<< "second=" << kerningInfo.secondId << " "
				<< "amount=" << kerningInfo.kerning * scalingFactor << std::endl;
		}

		// close file
		fntFile.close();
	}
}

