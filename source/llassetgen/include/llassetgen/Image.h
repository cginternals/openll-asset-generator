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
        uint8_t* data;
        bool isOwnerOfData;

        LLASSETGEN_NO_EXPORT Image(Vec2<size_t> _min, Vec2<size_t> _max, size_t _stride, uint8_t _bitDepth, uint8_t* _data);
        LLASSETGEN_NO_EXPORT static uint32_t reduceBitDepth(uint32_t in, uint8_t in_bitDepth, uint8_t out_bitDepth);
        LLASSETGEN_NO_EXPORT static size_t getFtBitdepth(const FT_Bitmap_& ft_bitmap);
        LLASSETGEN_NO_EXPORT static void readData(png_struct_def* png, uint8_t* data, size_t length);
        LLASSETGEN_NO_EXPORT static void writeData(png_struct_def* png, uint8_t* data, size_t length);
        LLASSETGEN_NO_EXPORT static void flushData(png_struct_def* png);

       public:
        ~Image();
        Image(size_t width, size_t height, size_t _bitDepth);
        Image view(Vec2<size_t> _min, Vec2<size_t> _max);
        size_t getWidth() const;
        size_t getHeight() const;
        size_t getBitDepth() const;
        bool isValid(Vec2<size_t> pos) const;
        template <typename pixelType>
        pixelType getPixel(Vec2<size_t> pos) const;
        template <typename pixelType>
        void setPixel(Vec2<size_t> pos, pixelType data) const;

        void load(const FT_Bitmap_& ft_bitmap);
        Image(const std::string& filepath, uint8_t _bitDepth = 0);
        template<typename pixelType>
        void exportPng(const std::string& filepath, pixelType black = std::numeric_limits<pixelType>::min(), pixelType white = std::numeric_limits<pixelType>::max());
    };
}
