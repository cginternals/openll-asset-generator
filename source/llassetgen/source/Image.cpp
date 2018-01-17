#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <fstream>

#include <ft2build.h>
#include FT_IMAGE_H

#include <png.h>

#include <llassetgen/Image.h>

namespace llassetgen {

	Image::Image(const FT_Bitmap &ft_bitmap) {
		height = ft_bitmap.rows;
		width = ft_bitmap.width;
		min_x = 0;
		min_y = 0;
		max_x = width;
		max_y = height;

		switch (ft_bitmap.pixel_mode) {
			case FT_PIXEL_MODE_MONO:
				bit_depth = 1;
				break;
			case FT_PIXEL_MODE_GRAY2:
				// We haven't found a single font using this format, however.
				bit_depth = 2;
				break;
			case FT_PIXEL_MODE_GRAY4:
				// We haven't found a single font using this format, however.
				bit_depth = 4;
				break;	
			default:
				bit_depth = 8;
		}
		stride = size_t(std::ceil(float(width * bit_depth) / 8));

		data = std::shared_ptr<uint8_t>(new uint8_t[height * stride]);
		for (size_t y = 0; y < height; y++) {
			for (size_t x = 0; x < stride; x++){
				data.get()[y * stride + x] = 0xFF - ft_bitmap.buffer[y * ft_bitmap.pitch + x];
			}
			size_t padding_bits = size_t((float(width * bit_depth) / 8 - std::floor(float(width * bit_depth) / 8)) * 8);
			uint8_t mask = ~(0xFF >> padding_bits);
			data.get()[y * stride + stride - 1] &= mask;
		}
	}

	Image::Image(const size_t _width, const size_t _height, const size_t _bit_depth) {
		assert(_bit_depth == 1 || _bit_depth == 2 || _bit_depth == 4 || _bit_depth == 8 || _bit_depth == 16 || _bit_depth == 24 || _bit_depth == 32);
		width = _width;
		height = _height;
		bit_depth = _bit_depth;
		stride = _width * (bit_depth / 8);
		min_x = 0;
		min_y = 0;
		max_x = width;
		max_y = height;
		data = std::shared_ptr<uint8_t>(new uint8_t[height * stride]());
	}

	size_t Image::get_width() {
		return width;
	}

	size_t Image::get_height() {
		return height;
	}

	size_t Image::get_bit_depth() {
		return bit_depth;
	}

	Image Image::view(const size_t _min_x, const size_t _max_x, const size_t _min_y, const size_t _max_y) {
		return Image(_min_x, _max_x, _min_y, _max_y, bit_depth, stride, data);
	}

	Image::Image(const size_t _min_x, const size_t _max_x, const size_t _min_y, const size_t _max_y, const uint8_t _bit_depth, const size_t _stride, const std::shared_ptr<uint8_t>_data)
	{
		width = _max_x - _min_x;
		height = _max_y - _min_y;
		stride = _stride;
		data = _data;
		min_x = _min_x;
		min_y = _min_y;
		max_x = _max_x;
		max_y = _max_y;
		bit_depth = _bit_depth;
	}

	uint32_t Image::at(const size_t x, const size_t y) {
 		size_t offset_x = x + min_x;
		size_t offset_y = y + min_y;
		assert(offset_x < max_x && offset_x >= min_x && offset_y < max_y && offset_y >= min_y);
		if (bit_depth <= 8) {
			uint8_t byte = data.get()[offset_y * stride + offset_x * bit_depth / 8];
			size_t bit_pos = offset_x % (8 / bit_depth);
			byte <<= bit_pos * bit_depth;
			byte >>= 8 - bit_depth;
			return (uint32_t)byte;
		} else {
			uint32_t return_data = 0;
			for (size_t byte_pos = 0; byte_pos < bit_depth / 8; ++byte_pos) {
				return_data <<= 8;
				return_data |= data.get()[offset_y * stride + offset_x * bit_depth / 8 + byte_pos];
			}
			return return_data;
		}
	}

	void Image::put(const size_t x, const size_t y, const uint32_t in) {
		size_t offset_x = x + min_x;
		size_t offset_y = y + min_y;
		assert(offset_x < max_x && offset_x >= min_x && offset_y < max_y && offset_y >= min_y);
		if (bit_depth <= 8) {
			uint8_t mask = 0xFF;
			uint8_t in_byte = (uint8_t)in;
			mask <<= 8 - bit_depth;
			in_byte <<= 8 - bit_depth;

			mask >>= 8 - bit_depth;
			in_byte >>= 8 - bit_depth;

			size_t bit_pos = offset_x % (8 / bit_depth);
			in_byte <<= 8 - bit_pos * bit_depth - bit_depth;
			mask <<= 8 - bit_pos * bit_depth - bit_depth;

			data.get()[offset_y * stride + offset_x * bit_depth / 8] = data.get()[offset_y * stride + offset_x * bit_depth / 8] & ~mask | in_byte;
		} else {
			uint32_t in_int = in;
			for (int byte_pos = bit_depth / 8 - 1; byte_pos >= 0; byte_pos--) {
				data.get()[offset_y * stride + offset_x * bit_depth / 8 + byte_pos] = (uint8_t)in_int;
				in_int >>= 8;
			}
		}
	}

	void Image::exportPng(const std::string filepath) {
		std::ofstream out_file(filepath, std::ofstream::out | std::ofstream::binary);
		if (!out_file.good()) {
			std::cerr << "could not open file " << filepath;
			abort();
		}

		png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (!png) {
			std::cerr << "failed to create png write struct";
			abort();
		}

		png_infop info = png_create_info_struct(png);
		if (!info) {
			png_destroy_write_struct(&png, (png_infopp)0);
			std::cerr << "failed to create png info struct";
			abort();
		}

		if (setjmp(png_jmpbuf(png))){
			std::cerr << "pnglib caused a longjump due to an error" << std::endl;
			std::cerr << "could not write file " << filepath;
			abort();
		}

		png_set_write_fn(png, (png_voidp)&out_file, write_data, flush_data);

		png_set_IHDR(	png, 
						info, 
						width, 
						height,
						bit_depth, 
						PNG_COLOR_TYPE_GRAY, 
						PNG_INTERLACE_NONE,
						PNG_COMPRESSION_TYPE_BASE, 
						PNG_FILTER_TYPE_BASE);

		png_write_info(png, info);

		if (bit_depth > 8) {
			png_set_swap(png);
		}
	
		for (int y = 0; y < height; y++) {
			png_write_row(png, reinterpret_cast<png_bytep>(&data.get()[y * stride]));
		}
	
		png_write_end(png, nullptr);

		png_destroy_write_struct(&png, &info);
		out_file.close();
	}

	void Image::read_data(png_structp png, png_bytep data, png_size_t length) {
		png_voidp a = png_get_io_ptr(png);
		((std::istream*)a)->read((char*)data, length);
	}

	void Image::write_data(png_structp png, png_bytep data, png_size_t length) {
		png_voidp a = png_get_io_ptr(png);
		((std::ostream*)a)->write((char*)data, length);
	}
	
	void Image::flush_data(png_structp png) {
		png_voidp a = png_get_io_ptr(png);
		((std::ostream*)a)->flush();
	}

	Image::Image(const std::string filepath)
	{
		std::ifstream in_file(filepath, std::ifstream::in | std::ifstream::binary);
		
		png_byte pngsig[8];
		in_file.read((char*)pngsig, 8);
		if (!in_file.good()) {
			std::cerr << "could not read from file";
			abort();
		}
		if (png_sig_cmp(pngsig, 0, 8)) {
			std::cerr << "no PNG file signature";
			abort();
		}

		png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png) {
			std::cerr << "failed to create png read struct";
			abort();
		}

		png_infop info = png_create_info_struct(png);
		if (!info) {
			png_destroy_read_struct(&png, (png_infopp)0, (png_infopp)0);
			std::cerr << "failed to create png info struct";
			abort();
		}

		if (setjmp(png_jmpbuf(png))) {
			std::cerr << "pnglib caused a longjump due to an error";
			abort();
		}

		png_set_read_fn(png, (png_voidp)&in_file, read_data);

		png_set_sig_bytes(png, 8);

		png_read_info(png, info);

		width = png_get_image_width(png, info);
		height = png_get_image_height(png, info);
		min_x = 0;
		min_y = 0;
		max_x = width;
		max_y = height;
		bit_depth = png_get_bit_depth(png, info);
		uint32_t color_type = png_get_color_type(png, info);

		if (color_type == PNG_COLOR_TYPE_GRAY) {
			if (bit_depth < 8)
				png_set_expand_gray_1_2_4_to_8(png);
			bit_depth = 8;
		}
		else {
			if (png_get_valid(png, info, PNG_INFO_tRNS)) {
				png_set_tRNS_to_alpha(png);
			}
			if (color_type == PNG_COLOR_TYPE_PALETTE) {
				png_set_palette_to_rgb(png);
			}
			else	if (color_type == PNG_COLOR_TYPE_RGB ||
				color_type == PNG_COLOR_TYPE_RGBA ||
				color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
				png_set_rgb_to_gray_fixed(png, 1, -1, -1);
			}
			/* TODO: all color types:
			PNG_COLOR_TYPE_GRAY_ALPHA
			PNG_COLOR_MASK_PALETTE
			PNG_COLOR_MASK_COLOR
			PNG_COLOR_MASK_ALPHA*/
		}

		png_read_update_info(png, info);
		uint8_t channels = png_get_channels(png, info);
		
		stride = width * (bit_depth / 8);

		std::unique_ptr<png_bytep> row_ptrs(new png_bytep[height]);
		std::unique_ptr<uint8_t> multi_channel_data(new uint8_t[height * stride * channels]);
		for (size_t i = 0; i < height; i++) {
			row_ptrs.get()[i] = (png_bytep)multi_channel_data.get() + i * stride * channels;
		}

		png_read_image(png, row_ptrs.get());

		png_destroy_read_struct(&png, &info, (png_infopp)0);
		in_file.close();

		data = std::shared_ptr<uint8_t>(new uint8_t[height * stride]);
		for (size_t y = 0; y < height; y++) {
			for (size_t x = 0; x < width; x++) {
				put(x, y, multi_channel_data.get()[y * stride * channels + x * bit_depth / 8 * channels]);
			}
		}
	}
}
