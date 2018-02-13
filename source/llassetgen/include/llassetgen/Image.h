#pragma once

#include <llassetgen/Vec2.h>

#include <limits>
#include <string>
#include <memory>

struct FT_Bitmap_;
struct png_struct_def;

namespace llassetgen {
	class Image {
		public:
			LLASSETGEN_API Image(const std::string &filepath);
			LLASSETGEN_API Image(const size_t width, const size_t height, const size_t bit_depth);
			template<typename pixelType>
			LLASSETGEN_API void exportPng(const std::string & filepath, pixelType black = std::numeric_limits<pixelType>::min(), pixelType white = std::numeric_limits<pixelType>::max());
            LLASSETGEN_API bool is_valid(const Vec2<size_t> pos) const;
        	template <typename pixelType>
			LLASSETGEN_API pixelType getPixel(const Vec2<size_t> pos) const;
			template <typename pixelType>
			LLASSETGEN_API void setPixel(const Vec2<size_t> pos, const pixelType data) const;
			LLASSETGEN_API Image(const FT_Bitmap_ & ft_bitmap);
			LLASSETGEN_API void load(const FT_Bitmap_ & ft_bitmap);
			LLASSETGEN_API size_t get_width() const;
			LLASSETGEN_API size_t get_height() const;
			LLASSETGEN_API size_t get_bit_depth() const;
			LLASSETGEN_API Image view(const Vec2<size_t> min, const Vec2<size_t> max);
		private:
			Image(const Vec2<size_t> min, const Vec2<size_t> max, const size_t _stride, const uint8_t _bit_depth, const std::shared_ptr<uint8_t> _data);
            Vec2<size_t> min, max;
            size_t stride;
			uint8_t bit_depth;
			std::shared_ptr<uint8_t> data;
			static void read_data(png_struct_def* png, uint8_t* data, size_t length);
			static void write_data(png_struct_def* png, uint8_t* data, size_t length);
			static void flush_data(png_struct_def* png);
	};
}
