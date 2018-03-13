#pragma once

#include <llassetgen/llassetgen_api.h>
#include <llassetgen/Geometry.h>

#include <limits>
#include <string>
#include <memory>

struct FT_Bitmap_;
struct png_struct_def;

namespace llassetgen {
    class LLASSETGEN_API Image {
        Vec2<size_t> min, max;
        size_t stride;
        uint8_t bitDepth;
        std::shared_ptr<uint8_t> data;
        uint16_t reduceBitDepth(uint16_t in, uint8_t in_bitDepth, uint8_t out_bitDepth);
        LLASSETGEN_NO_EXPORT Image(const Vec2<size_t> min, const Vec2<size_t> max, const size_t _stride, const uint8_t _bitDepth, const std::shared_ptr<uint8_t> _data);
        LLASSETGEN_NO_EXPORT static void readData(png_struct_def* png, uint8_t* data, size_t length);
        LLASSETGEN_NO_EXPORT static void writeData(png_struct_def* png, uint8_t* data, size_t length);
        LLASSETGEN_NO_EXPORT static void flushData(png_struct_def* png);
       public:
        Image(const std::string & filepath, uint8_t _bitDepth = 0);
        Image(const size_t width, const size_t height, const uint8_t bitDepth);
        template<typename pixelType>
        void exportPng(const std::string & filepath, pixelType black = std::numeric_limits<pixelType>::min(), pixelType white = std::numeric_limits<pixelType>::max());
        template <typename pixelType>
        pixelType getPixel(const Vec2<size_t> pos) const;
        template <typename pixelType>
        void setPixel(const Vec2<size_t> pos, const pixelType data) const;
        Image(const FT_Bitmap_ & ft_bitmap, size_t padding = 0);
        bool isValid(const Vec2<size_t> pos) const;
        void load(const FT_Bitmap_ & ft_bitmap, size_t padding);
        size_t getWidth() const;
        size_t getHeight() const;
        size_t getBitDepth() const;
        Image view(const Vec2<size_t> min, const Vec2<size_t> max);
    };
}
