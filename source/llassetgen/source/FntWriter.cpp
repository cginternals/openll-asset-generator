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
	FntWriter::FntWriter(FT_Face _face, std::string _face_name) {
		face_name = _face_name;
		face = _face;
		font_info = Info();
		font_common = Common();
		char_infos = std::vector<CharInfo>();
		kerning_infos = std::vector<KerningInfo>();
	}

	void FntWriter::setFontInfo() {
		// collect font_info 
		font_info.face = face->family_name + std::string(" ") + face->style_name;
		// todo: determine real font size from size object.
		font_info.size = face->size->metrics.height;
		font_info.is_bold == false;
		font_info.is_italic == false;
		if (face->style_flags == FT_STYLE_FLAG_ITALIC) {
			font_info.is_italic == true;
		}
		if (face->style_flags == FT_STYLE_FLAG_BOLD) {
			font_info.is_bold == true;
		}
		font_info.charset = face->charmap->encoding;
		if (face->charmap->encoding == FT_ENCODING_UNICODE) {
			font_info.use_unicode = true;
		}
		else {
			font_info.use_unicode = false;
		}
		// havent found any of the following information
		// set to default values
		font_info.stretch_h = 1.0f;
		font_info.use_smoothing = false;
		font_info.supersampling_level = 1;
		font_info.padding = { 2.8f, 2.8f, 2.8f, 2.8f };
		font_info.spacing = { 0, 0 };
		font_info.outline_thickness = 0;
	}

	void FntWriter::setCharInfo(FT_UInt gindex, Vec2<float> position, Vec2<float> size, Vec2<float> offset) {
		FT_Load_Glyph(face,	gindex, FT_LOAD_DEFAULT);
		CharInfo char_info;
		char_info.id = gindex;
		char_info.x = position.x;
		char_info.y = position.y;
		char_info.width = size.x;
		char_info.height = size.y;
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
				// this returns the kerning in font units
				// How to get float values from that???
				FT_Get_Kerning(face, left_gindex, right_gindex, FT_KERNING_UNSCALED, &kerning_vector);
				if (kerning_vector.x != 0) {
					KerningInfo kerning_info;
					kerning_info.first_id = left_gindex;
					kerning_info.second_id = right_gindex;
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

	void FntWriter::setAtlasProperties(Image atlas) {
		// collect common_info
		font_common.line_height = 0; // dont know where to get that
		font_common.base = 0; // dont know where to get that
		font_common.scale_w = atlas.getWidth();
		font_common.scale_h = atlas.getHeight();
		font_common.pages = 1;
		font_common.is_packed = 0;
	}
	
	void FntWriter::SaveFnt(std::string filepath) {
		// check for correct file ending
		if (filepath.substr(filepath.length() - 4) == ".fnt") {
			filepath = filepath.substr(0, filepath.length() - 4);
		}

		// open file
		std::ofstream fnt_file;
		fnt_file.open(filepath + ".fnt");

		// cut off path from filename
		size_t last_slash_pos = filepath.rfind('\\');
		std::string filename;
		if (last_slash_pos != std::string::npos) {
			filename = filepath.substr(last_slash_pos + 1);
		}
		else {
			filename = filepath;
		}

		// write in plain text format
		// TODO: XML and binary

		// write info block
		fnt_file << "info "
			<< "face=\"" << font_info.face << "\" "
			<< "size=" << font_info.size << " "
			<< "bold=" << int(font_info.is_bold) << " "
			<< "italic=" << int(font_info.is_italic) << " "
			<< "charset=\"" << font_info.charset << "\" "
			<< "unicode=" << int(font_info.use_unicode) << " "
			<< "stretchH=" << font_info.stretch_h << " "
			<< "smooth=" << int(font_info.use_smoothing) << " "
			<< "aa=" << font_info.supersampling_level << " "
			<< "padding=" << font_info.padding.up << "," << font_info.padding.right << "," << font_info.padding.down << "," << font_info.padding.left << " "
			<< "spacing=" << font_info.spacing.horiz << "," << font_info.spacing.vert << " "
			<< "outline=" << font_info.outline_thickness << std::endl;

		// write common block
		fnt_file << "common "
			<< "lineHeight=" << font_common.line_height << " "
			<< "base=" << font_common.base << " "
			<< "scaleW=" << font_common.scale_w << " "
			<< "scaleH=" << font_common.scale_h << " "
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

		// write infor for each char
		for (auto char_info : char_infos) {
			fnt_file << "char "
				<< "id=" << char_info.id << " "
				<< "x=" << char_info.x << " "
				<< "y=" << char_info.y << " "
				<< "width=" << char_info.width << " "
				<< "height=" << char_info.height << " "
				<< "xoffset=" << char_info.x_offset << " "
				<< "yoffset=" << char_info.y_offset << " "
				<< "xadvance=" << char_info.x_advance << " "
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
				<< "amount=" << kerning_info.kerning << std::endl;
		}

		// close file
		fnt_file.close();
	}
}