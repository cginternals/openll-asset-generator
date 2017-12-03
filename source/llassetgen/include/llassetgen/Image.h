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
			LLASSETGEN_API Image(FT_Bitmap &ft_bitmap);
			LLASSETGEN_API Image(std::string filepath);
			LLASSETGEN_API void exportPng(std::string filepath);
			LLASSETGEN_API int at(size_t x, size_t y);
		private:
			uint32_t height;
			uint32_t width;
			unsigned char bit_depth;
			std::unique_ptr<uint8_t> data;
			static void read_data(png_structp png, png_bytep data, png_size_t length);
			static void write_data(png_structp png, png_bytep data, png_size_t length);
			static void flush_data(png_structp png);
	};
}