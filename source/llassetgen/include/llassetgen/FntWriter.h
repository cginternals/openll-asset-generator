#pragma once

#include <string>
#include <vector>

#include <llassetgen/Image.h>

#include <ft2build.h>
#include <llassetgen/packing/Types.h>
#include FT_FREETYPE_H

namespace llassetgen {
	struct Info {
		std::string face;
		int size;
		bool is_bold;
		bool is_italic;
		std::string charset;
		bool use_unicode;
		/*
		float stretch_h;
		bool use_smoothing;
		int supersampling_level;
		struct padding {
			float left;
			float right;
			float up;
			float down;
		} padding;
		struct spacing {
			float horiz;
			float vert;
		} spacing;
		float outline_thickness;
		*/
	};

	struct Common {
		int line_height;
		int base;
		PackingSizeType scale_w;
		PackingSizeType scale_h;
		int pages;
		bool is_packed;
		uint8_t alpha_chnl;
		uint8_t red_chnl;
		uint8_t green_chnl;
		uint8_t blue_chnl;
	};

	struct CharInfo {
		int id;
		PackingSizeType x;
		PackingSizeType y;
		PackingSizeType width;
		PackingSizeType height;
		float x_offset;
		float y_offset;
		float x_advance;
		int page;
		uint8_t chnl;
	};

	struct KerningInfo {
		int first_id;
		int second_id;
		float kerning;
	};

	class LLASSETGEN_API FntWriter {
		public:
			FntWriter(FT_Face face, std::string face_name, unsigned int font_size, float scaling_factor, bool scaled_glyph);
			void readFont();
			void setAtlasProperties(Vec2<PackingSizeType> size, int max_height);
			void saveFnt(std::string filepath);
			void setFontInfo();
			void setCharInfo(FT_UInt charcode, Rect<PackingSizeType> char_area, Vec2<float> offset);
			void setKerningInfo();
		private:
			FT_Face face;
			std::string face_name;
			Info font_info;
			Common font_common;
			std::vector<CharInfo> char_infos;
			std::vector<KerningInfo> kerning_infos;
			FT_Pos max_y_bearing; 
			float scaling_factor;
			bool scaled_glyph;
	};
}