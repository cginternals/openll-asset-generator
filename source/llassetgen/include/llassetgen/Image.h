#pragma once

#include <llassetgen/llassetgen_api.h>
#include <llassetgen/llassetgen.h>

#include <vector>
#include <string>
#include <memory>

#include <png.h>

#include <ft2build.h>
#include FT_IMAGE_H

namespace llassetgen {
	class Image {
		public:
			LLASSETGEN_API Image(const std::string &filepath);
			LLASSETGEN_API Image(const size_t width, const size_t height, const size_t bit_depth);
			template<typename pixelType>
			LLASSETGEN_API void exportPng(const std::string & filepath, pixelType min = std::numeric_limits<pixelType>::min(), pixelType max = std::numeric_limits<pixelType>::max());
			template <typename pixelType>
			LLASSETGEN_API pixelType at(const size_t x, const size_t y);
			template <typename pixelType>
			LLASSETGEN_API void put(const size_t x, const size_t y, const pixelType data);
			LLASSETGEN_API Image(const FT_Bitmap & ft_bitmap);
			LLASSETGEN_API void load(const FT_Bitmap & ft_bitmap);
			LLASSETGEN_API size_t get_width();
			LLASSETGEN_API size_t get_height();
			LLASSETGEN_API size_t get_bit_depth();
			LLASSETGEN_API Image view(const size_t _min_x, const size_t _max_x, const size_t _min_y, const size_t _max_y);
		private:
			Image(const size_t _min_x, const size_t _max_x, const size_t _min_y, const size_t _max_y, const uint8_t _bit_depth, const size_t _stride, const std::shared_ptr<uint8_t> _data);
			size_t stride;
			size_t height, width;
			size_t min_x, max_x, min_y, max_y;
			uint8_t bit_depth;
			std::shared_ptr<uint8_t> data;
			static void read_data(png_structp png, png_bytep data, png_size_t length);
			static void write_data(png_structp png, png_bytep data, png_size_t length);
			static void flush_data(png_structp png);
	};
}
