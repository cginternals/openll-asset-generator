#pragma once

#include <llassetgen/Geometry.h>
#include <llassetgen/llassetgen_api.h>

#include <limits>
#include <memory>
#include <string>

struct FT_Bitmap_;
struct png_struct_def;

namespace llassetgen {
    class LLASSETGEN_API Image {
        Vec2<size_t> min, max;
        size_t stride;
        uint8_t bitDepth;
        uint8_t* data;
        bool isOwnerOfData;

        LLASSETGEN_NO_EXPORT Image(Vec2<size_t> _min, Vec2<size_t> _max, size_t _stride, uint8_t _bitDepth,
                                   uint8_t* _data);
        LLASSETGEN_NO_EXPORT static uint32_t reduceBitDepth(uint32_t in, uint8_t in_bitDepth, uint8_t out_bitDepth);
        LLASSETGEN_NO_EXPORT static size_t getFtBitdepth(const FT_Bitmap_& ft_bitmap);
        LLASSETGEN_NO_EXPORT static void readData(png_struct_def* png, uint8_t* data, size_t length);
        LLASSETGEN_NO_EXPORT static void writeData(png_struct_def* png, uint8_t* data, size_t length);
        LLASSETGEN_NO_EXPORT static void flushData(png_struct_def* png);

        LLASSETGEN_NO_EXPORT void fillPadding(size_t padding);

       public:
        ~Image();
        Image& operator=(const Image&) = delete;
        Image(const Image&) = delete;
        Image(Image&& src);
        Image(size_t width, size_t height, size_t _bitDepth);
        Image(FT_Bitmap_ bitmap, size_t padding = 0);
        Image view(Vec2<size_t> _min, Vec2<size_t> _max, size_t padding = 0);

        size_t getWidth() const;
        size_t getHeight() const;
        size_t getBitDepth() const;
        Vec2<size_t> getSize() const;

        bool isValid(Vec2<size_t> pos) const;
        template <typename pixelType>
        pixelType getPixel(Vec2<size_t> pos) const;
        template <typename pixelType>
        void setPixel(Vec2<size_t> pos, pixelType data) const;

        template <typename pixelType = uint8_t>
        void fillRect(Vec2<size_t> _min, Vec2<size_t> _max, pixelType in = 0) const;
        void clear() const;
        void copyDataFrom(const Image& copy);

        template <typename pixelType>
        void centerDownsampling(const Image& src) const;
        template <typename pixelType>
        void averageDownsampling(const Image& src) const;
        template <typename pixelType>
        void minDownsampling(const Image& src) const;

        void load(const FT_Bitmap_& ft_bitmap);
        Image(const std::string& filepath, uint8_t _bitDepth = 0);
        template <typename pixelType>
        void exportPng(const std::string& filepath,
                       pixelType black = std::numeric_limits<pixelType>::min(),
                       pixelType white = std::numeric_limits<pixelType>::max());
    };
}
