#include <vector>
#include <string>
#include <memory>
#include <iostream>

#include <ft2build.h>
#include FT_IMAGE_H

#include <png.h>

#include <llassetgen/Bitmap.h>

namespace llassetgen {

	template<typename T>
	Bitmap<T>::Bitmap(FT_Bitmap &ft_bitmap) {
		height = ft_bitmap.rows;
		width = ft_bitmap.width;
		pitch = ft_bitmap.pitch;
		data.reserve(ft_bitmap.rows * pitch * width);
		for (int i = 0; i < ft_bitmap.rows * pitch * width; i++) {
			data.push_back(ft_bitmap.buffer[i]);
		}
	}

	/*template<typename T>
	void Bitmap<T>::print() {
		for (int x = 0; x < height; x++) {
			for (int y = 0; y < width; y++) {
				std::cout << (this[x][y] > 0) ? "#" : " ";
			}
			std::cout << std::endl;
		}
	}
	*/
	/*
	template<typename T>
	Bitmap<T>::Bitmap(std::string filepath)
	{
		FILE* file = fopen(filepath.c_str(), "r");

		png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png) abort();

		png_infop info = png_create_info_struct(png);
		if (!info) abort();

		if (setjmp(png_jmpbuf(png))) abort();

		png_init_io(png, fp);

		png_read_info(png, info);

		width = png_get_image_width(png, info);
		pitch = width;
		height = png_get_image_height(png, info);

		color_type = png_get_color_type(png, info);
		bit_depth = png_get_bit_depth(png, info);

		if (bit_depth == 16)
			png_set_strip_16(png);

		if (color_type == PNG_COLOR_TYPE_PALETTE)
			png_set_palette_to_rgb(png);

		if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
			png_set_expand_gray_1_2_4_to_8(png);

		if (png_get_valid(png, info, PNG_INFO_tRNS))
			png_set_tRNS_to_alpha(png);

		if (color_type == PNG_COLOR_TYPE_RGB ||
			color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_PALETTE)
			png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

		if (color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(png);

		png_read_update_info(png, info);


		// TODO ...

		std::unique_ptr<png_byte[]> row_pointers(new png_byte[width]);
		for (int y = 0; y < height; y++) {
			png_read_row(png, reinterpret_cast<png_bytep>(row_pointers.get()), NULL);
			for (int x = 0; x < width; x++) {
				data.push_back(rowBuffer[x]);
			}
		}

		png_read_end(png, NULL);
		png_destroy_read_struct(&png, &pngInfo, NULL);
		fclose(file);
	}
	*/

	template<typename T>
	T & Bitmap<T>::operator[](std::size_t index) {
		return std::vector<T>(data.begin() + index * pitch, data.begin() + index * pitch + pitch - 1);
	}

	template<typename T>
	const T & Bitmap<T>::operator[](std::size_t index) const {
		return std::vector<T>(data.begin() + index * pitch, data.begin() + index * pitch + pitch - 1);
	}
}
