#include <fstream> 
#include <float.h>
#include <string>
#include <vector>
#include <cassert>
#include <iostream>

#include <llassetgen/FntWriter.h>
#include <llassetgen/Image.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

namespace llassetgen {
	FntWriter::FntWriter(FT_Face _face, std::string _face_name, int _font_size, float _scaling_factor, bool _scaled_glyph) {
		face_name = _face_name;
		face = _face;
		font_info = Info();
		font_common = Common();
		char_infos = std::vector<CharInfo>();
		kerning_infos = std::vector<KerningInfo>();
		font_info.size = _font_size;
		max_y_bearing = 0;
		scaling_factor = _scaling_factor;
		scaled_glyph = _scaled_glyph;
	}

	void FntWriter::setFontInfo() {
		// collect font_info 
		font_info.face = face->family_name + std::string(" ") + face->style_name;
		font_info.is_bold = false;
		font_info.is_italic = false;
		if (face->style_flags == FT_STYLE_FLAG_ITALIC) {
			font_info.is_italic = true;
		}
		if (face->style_flags == FT_STYLE_FLAG_BOLD) {
			font_info.is_bold = true;
		}
		int encoding = int(face->charmap->encoding);
		uint8_t *character = reinterpret_cast<uint8_t*>(&encoding);
		for (int i = 3; i >= 0; --i) {
			font_info.charset += char(character[i]);
		}

		font_info.use_unicode = (face->charmap->encoding == FT_ENCODING_UNICODE);

		// havent found any of the following information
		// irrelevant for distance fields --> ignore them
		/*
		font_info.stretch_h = 1.0f;
		font_info.use_smoothing = false;
		font_info.supersampling_level = 1;
		font_info.padding = { 2.8f, 2.8f, 2.8f, 2.8f };
		font_info.spacing = { 0, 0 };
		font_info.outline_thickness = 0;
		*/
	}

	void FntWriter::setCharInfo(FT_UInt gindex, Rect<uint32_t> char_area, Vec2<float> offset) {
		FT_Load_Glyph(face,	gindex, FT_LOAD_DEFAULT);
		
		FT_Pos y_bearing = face->glyph->metrics.vertBearingY;
		max_y_bearing = std::max(y_bearing, max_y_bearing);

		CharInfo char_info;
		char_info.id = gindex;
		char_info.x = char_area.position.x;
		char_info.y = char_area.position.y;
		char_info.width = char_area.size.x;
		char_info.height = char_area.size.y;
		char_info.x_advance = float(face->glyph->linearHoriAdvance) * 16.16f;
		char_info.x_offset = offset.x;
		char_info.y_offset = offset.y;
		char_info.page = 1;
		char_info.chnl = 15;
		char_infos.push_back(char_info);
	}

	void FntWriter::setKerningInfo() {
		FT_ULong  left_charcode;
		FT_UInt   left_gindex;
		left_charcode = FT_Get_First_Char(face, &left_gindex);
		while (left_gindex != 0) {
			FT_ULong  right_charcode;
			FT_UInt   right_gindex;
			right_charcode = FT_Get_First_Char(face, &right_gindex);
			while (right_gindex != 0) {
				FT_Vector kerning_vector;
				FT_Get_Kerning(face, left_gindex, right_gindex, FT_KERNING_UNSCALED, &kerning_vector);
				if (kerning_vector.x != 0) {
					KerningInfo kerning_info;
					kerning_info.first_id = left_gindex;
					kerning_info.second_id = right_gindex;
					// kerning is provided in 26.6 fractions of a pixel
					kerning_info.kerning = float(kerning_vector.x) / 26.6f;
					kerning_infos.push_back(kerning_info);
				}
				right_charcode = FT_Get_Next_Char(face, right_charcode, &right_gindex);
			}

			left_charcode = FT_Get_Next_Char(face, left_charcode, &left_gindex);
		}
	}

	void FntWriter::readFont() {
		setFontInfo();
		setKerningInfo();
	}

	void FntWriter::setAtlasProperties(Vec2<PackingSizeType> size, int max_height) {
		// collect common_info
		font_common.line_height = max_height;
		font_common.scale_w = size.x;
		font_common.scale_h = size.y;
		font_common.pages = 1;
		font_common.is_packed = 0;
	}
	
	void FntWriter::saveFnt(std::string filepath) {
		font_common.base = max_y_bearing;

		// open file
		std::ofstream fnt_file;
		fnt_file.open(filepath);

		// write in plain text format
		// TODO: XML and binary

		// write info block
		fnt_file << "info "
			<< "face=\"" << font_info.face << "\" "
			<< "size=" << font_info.size << " "
			<< "bold=" << int(font_info.is_bold) << " "
			<< "italic=" << int(font_info.is_italic) << " "
			<< "charset=\"" << font_info.charset << "\" "
			<< "unicode=" << int(font_info.use_unicode) 
			/* << " "
			<< "stretchH=" << font_info.stretch_h << " "
			<< "smooth=" << int(font_info.use_smoothing) << " "
			<< "aa=" << font_info.supersampling_level << " "
			<< "padding=" << font_info.padding.up << "," << font_info.padding.right << "," << font_info.padding.down << "," << font_info.padding.left << " "
			<< "spacing=" << font_info.spacing.horiz << "," << font_info.spacing.vert << " "
			<< "outline=" << font_info.outline_thickness
			*/
			<< std::endl;

		// write common block
		fnt_file << "common "
			<< "lineHeight=" << float(font_common.line_height) * scaling_factor << " "
			<< "base=" << float(font_common.base) * scaling_factor << " "
			<< "scaleW=" << (scaled_glyph ? (float(font_common.scale_w) * scaling_factor) : font_common.scale_w) << " "
			<< "scaleH=" << (scaled_glyph ? (float(font_common.scale_h) * scaling_factor) : font_common.scale_h) << " "
			<< "pages=" << font_common.pages << " "
			<< "packed=" << int(font_common.is_packed) << std::endl;

		// write page files
		for (int i = 0; i < font_common.pages; i++) {
			fnt_file << "page "
				<< "id=" << i << " "
				<< "file=\"" << face_name << "\"" << std::endl;
		}

		// write char count
		fnt_file << "chars count=" << char_infos.size() << std::endl;

		// write info for each char
		for (auto char_info : char_infos) {
			fnt_file << "char "
				<< "id=" << char_info.id << " "
				<< "x=" << char_info.x << " "
				<< "y=" << char_info.y << " "
				<< "width=" << (scaled_glyph ? (float(char_info.width) * scaling_factor) : char_info.width) << " "
				<< "height=" << (scaled_glyph ? (float(char_info.height) * scaling_factor) : char_info.height) << " "
				<< "xoffset=" << char_info.x_offset << " "
				<< "yoffset=" << char_info.y_offset << " "
				<< "xadvance=" << float(char_info.x_advance) * scaling_factor << " "
				<< "page=" << char_info.page << " "
				<< "chnl=" << int(char_info.chnl) << std::endl;
		}

		// write kerning count
		fnt_file << "kernings count=" << kerning_infos.size() << std::endl;

		// write each kerning info
		for (auto kerning_info : kerning_infos) {
			fnt_file << "kerning "
				<< "first=" << kerning_info.first_id << " "
				<< "second=" << kerning_info.second_id << " "
				<< "amount=" << kerning_info.kerning * scaling_factor << std::endl;
		}

		// close file
		fnt_file.close();
	}
}
