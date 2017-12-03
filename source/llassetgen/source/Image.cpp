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

	Image::Image(FT_Bitmap &ft_bitmap) {
		height = ft_bitmap.rows;
		width = ft_bitmap.width;

		// TODO: correct mono mode
		switch (ft_bitmap.pixel_mode) {
			case FT_PIXEL_MODE_MONO:
				bit_depth = 1;
				break;
			case FT_PIXEL_MODE_GRAY2:
				bit_depth = 2;
				break;
			case FT_PIXEL_MODE_GRAY4:
				bit_depth = 4;
				break;	
			default:
				bit_depth = 8;
		}
		data = std::unique_ptr<uint8_t>(new uint8_t[height * width * bit_depth / 8]);
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++){
				for (size_t byte = 0; byte < bit_depth / 8; byte++) {
					data.get()[y * width * bit_depth / 8 + x * bit_depth / 8 + byte] = ft_bitmap.buffer[y * ft_bitmap.pitch * bit_depth / 8 + x * bit_depth / 8 + byte];
				}
			}
		}
	}

	int Image::at(size_t x, size_t y) {
		int pixel = 0;
		for (int byte = 0; byte < bit_depth / 8; byte++) {
			pixel <<= 8;
			pixel |= data.get()[y * width * bit_depth / 8 + x * bit_depth / 8 + byte];
		}
		return pixel;
	}

	void Image::exportPng(std::string filepath) {
		std::ofstream out_file(filepath, std::ofstream::out | std::ofstream::binary);
		if (!out_file.good()) {
			std::cerr << "couldnt open file";
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
			std::cerr << "pnglib caused a longjump due to an error";
			abort();
		}

		png_set_write_fn(png, (png_voidp)&out_file, write_data, flush_data);

		png_set_IHDR(png, info, width, height,
			bit_depth, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_write_info(png, info);
		
		if (bit_depth > 8) {
			png_set_swap(png);
		}
	
		for (int y = 0; y < height; y++) {
			png_write_row(png, reinterpret_cast<png_bytep>(&data.get()[y * width * bit_depth / 8]));
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

	Image::Image(std::string filepath)
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
		bit_depth = png_get_bit_depth(png, info);
		uint32_t color_type = png_get_color_type(png, info);

		if (color_type == PNG_COLOR_TYPE_GRAY) {
			if (bit_depth < 8)
				png_set_expand_gray_1_2_4_to_8(png);
			bit_depth = 8;
		} else {
			if (png_get_valid(png, info, PNG_INFO_tRNS)) {
				png_set_tRNS_to_alpha(png);
			}
			if (color_type == PNG_COLOR_TYPE_PALETTE) {
				png_set_palette_to_rgb(png);
			} else	if (color_type == PNG_COLOR_TYPE_RGB ||
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
		
		uint8_t stride = width * bit_depth * channels / 8;

		std::unique_ptr<png_bytep> row_ptrs(new png_bytep[height]);
		std::unique_ptr<uint8_t> multi_channel_data(new uint8_t[width * height * bit_depth * channels / 8]);
		for (size_t i = 0; i < height; i++) {
			row_ptrs.get()[i] = (png_bytep)multi_channel_data.get() + i * stride;
		}

		png_read_image(png, row_ptrs.get());

		png_destroy_read_struct(&png, &info, (png_infopp)0);
		in_file.close();

		data = std::unique_ptr<uint8_t>(new uint8_t[width * height * bit_depth / 8]);
		for (size_t y = 0; y < height; y++) {
			for (size_t x = 0; x < width; x++) {
				for (int byte = 0; byte < bit_depth / 8; byte++) {
					data.get()[y * width * bit_depth / 8 + x * bit_depth / 8 + byte] = multi_channel_data.get()[y * stride + x * bit_depth / 8 * channels + byte];
				}
			}
		}
	}
}
