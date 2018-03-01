#pragma once

#include <llassetgen/Vec2.h>

#include <limits>
#include <string>
#include <memory>

struct FT_Bitmap_;
struct png_struct_def;

namespace llassetgen {
    class LLASSETGEN_API Image {
        Vec2<size_t> min, max;
        size_t stride;
        uint8_t bit_depth;
        std::shared_ptr<uint8_t> data;
		uint16_t reduce_bit_depth(uint16_t in, uint8_t in_bit_depth, uint8_t out_bit_depth);
        LLASSETGEN_NO_EXPORT Image(const Vec2<size_t> min, const Vec2<size_t> max, const size_t _stride, const uint8_t _bit_depth, const std::shared_ptr<uint8_t> _data);
        LLASSETGEN_NO_EXPORT static void read_data(png_struct_def* png, uint8_t* data, size_t length);
        LLASSETGEN_NO_EXPORT static void write_data(png_struct_def* png, uint8_t* data, size_t length);
        LLASSETGEN_NO_EXPORT static void flush_data(png_struct_def* png);
       public:
		Image(const std::string & filepath, int _bit_depth = -1);
        Image(const size_t width, const size_t height, const size_t bit_depth);
        template<typename pixelType>
        void exportPng(const std::string & filepath, pixelType black = std::numeric_limits<pixelType>::min(), pixelType white = std::numeric_limits<pixelType>::max());
        template <typename pixelType>
        pixelType getPixel(const Vec2<size_t> pos) const;
        template <typename pixelType>
        void setPixel(const Vec2<size_t> pos, const pixelType data) const;
        Image(const FT_Bitmap_ & ft_bitmap);
		bool is_valid(const Vec2<size_t> pos) const;
        void load(const FT_Bitmap_ & ft_bitmap);
        size_t get_width() const;
        size_t get_height() const;
        size_t get_bit_depth() const;
        Image view(const Vec2<size_t> min, const Vec2<size_t> max);
    };
}
