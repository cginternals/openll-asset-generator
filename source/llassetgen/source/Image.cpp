#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

/*
 * If we sort png.h below the freetype includes, an compile error will be
 * generated warning about multiple includes of setjmp (on some version of
 * libpng). Because we don't use the setjmp functionality of freetype, this is
 * not applicable in this case. Unfortunately, the compile error can only be
 * fixed by editing png.h or reordering the includes.
 *
 * See https://sourceforge.net/p/enlightenment/mailman/message/12595025/ for
 * some discussion on this topic.
 */
// clang-format off
#include <png.h> // NOLINT
#include <ft2build.h>
#include FT_FREETYPE_H
// clang-format on

#include <llassetgen/Image.h>

namespace llassetgen {
    Image::~Image() {
        if(isOwnerOfData)
            delete[] data;
    }

    Image::Image(Vec2<size_t> _min, Vec2<size_t> _max, size_t _stride, uint8_t _bitDepth, uint8_t* _data)
        : min(_min), max(_max), stride(_stride), bitDepth(_bitDepth), data(_data), isOwnerOfData(false) {}

    Image::Image(size_t width, size_t height, size_t _bitDepth)
        : min(Vec2<size_t>(0, 0)),
          max(Vec2<size_t>(width, height)),
          stride((width * _bitDepth + 7) / 8),
          bitDepth(_bitDepth),
          data(new uint8_t[stride * height]),
          isOwnerOfData(true) {}

    Image Image::view(Vec2<size_t> outerMin, Vec2<size_t> outerMax, size_t padding) {
        assert(outerMax.x <= getWidth() && outerMax.y <= getHeight());
        outerMin += min;
        outerMax += min;
        Vec2<size_t> paddingVec(padding, padding);
        return Image(outerMin + paddingVec, outerMax - paddingVec, stride, bitDepth, data);
    }

    size_t Image::getWidth() const { return max.x - min.x; }

    size_t Image::getHeight() const { return max.y - min.y; }

    size_t Image::getBitDepth() const { return bitDepth; }

    bool Image::isValid(Vec2<size_t> pos) const {
        Vec2<size_t> offset = min + pos;
        return offset.x >= min.x && offset.x < max.x && offset.y >= min.y && offset.y < max.y;
    }

    template LLASSETGEN_API float Image::getPixel<float>(Vec2<size_t> pos) const;
    template LLASSETGEN_API uint32_t Image::getPixel<uint32_t>(Vec2<size_t> pos) const;
    template LLASSETGEN_API uint16_t Image::getPixel<uint16_t>(Vec2<size_t> pos) const;
    template LLASSETGEN_API uint8_t Image::getPixel<uint8_t>(Vec2<size_t> pos) const;
    template <typename pixelType>
    pixelType Image::getPixel(Vec2<size_t> pos) const {
        assert(isValid(pos));
        Vec2<size_t> offset = min + pos;
        if (bitDepth <= 8) {
            uint8_t byte = data[offset.y * stride + offset.x * bitDepth / 8];
            size_t bit_pos = offset.x % (8 / bitDepth);
            byte <<= bit_pos * bitDepth;
            byte >>= 8 - bitDepth;
            uint32_t casted = static_cast<uint32_t>(byte);
            return reinterpret_cast<pixelType&>(casted);
        } else {
            uint32_t return_data = 0;
            for (size_t byte_pos = 0; byte_pos < bitDepth / 8; ++byte_pos) {
                return_data <<= 8;
                return_data |= data[offset.y * stride + offset.x * bitDepth / 8 + byte_pos];
            }
            return reinterpret_cast<pixelType&>(return_data);
        }
    }

    template LLASSETGEN_API void Image::setPixel<float>(Vec2<size_t> pos, float in) const;
    template LLASSETGEN_API void Image::setPixel<uint32_t>(Vec2<size_t> pos, uint32_t in) const;
    template LLASSETGEN_API void Image::setPixel<uint16_t>(Vec2<size_t> pos, uint16_t in) const;
    template LLASSETGEN_API void Image::setPixel<uint8_t>(Vec2<size_t> pos, uint8_t in) const;
    template <typename pixelType>
    void Image::setPixel(Vec2<size_t> pos, pixelType in) const {
        assert(isValid(pos));
        Vec2<size_t> offset = min + pos;
        if (bitDepth <= 8) {
            uint8_t mask = 0xFF;
            uint8_t in_byte = static_cast<uint8_t>(reinterpret_cast<uint32_t&>(in));
            mask <<= 8 - bitDepth;
            mask >>= 8 - bitDepth;

            size_t bit_pos = offset.x % (8 / bitDepth);
            in_byte <<= 8 - bit_pos * bitDepth - bitDepth;
            mask <<= 8 - bit_pos * bitDepth - bitDepth;
            data[offset.y * stride + offset.x * bitDepth / 8] =
                (data[offset.y * stride + offset.x * bitDepth / 8] & ~mask) | in_byte;
        } else {
            uint32_t in_int = reinterpret_cast<uint32_t&>(in);
            for (int byte_pos = bitDepth / 8 - 1; byte_pos >= 0; byte_pos--) {
                data[offset.y * stride + offset.x * bitDepth / 8 + byte_pos] = static_cast<uint8_t>(in_int);
                in_int >>= 8;
            }
        }
    }

    template LLASSETGEN_API void Image::fillRect<float>(Vec2<size_t> _min, Vec2<size_t> _max, float in) const;
    template LLASSETGEN_API void Image::fillRect<uint32_t>(Vec2<size_t> _min, Vec2<size_t> _max, uint32_t in) const;
    template LLASSETGEN_API void Image::fillRect<uint16_t>(Vec2<size_t> _min, Vec2<size_t> _max, uint16_t in) const;
    template LLASSETGEN_API void Image::fillRect<uint8_t>(Vec2<size_t> _min, Vec2<size_t> _max, uint8_t in) const;
    template <typename pixelType>
    void Image::fillRect(Vec2<size_t> _min, Vec2<size_t> _max, pixelType in) const {
        for (size_t y = _min.y; y < _max.y; y++) {
            for (size_t x = _min.x; x < _max.x; x++) {
                setPixel<pixelType>({x, y}, in);
            }
        }
    }

    void Image::clear() const {
        if(isOwnerOfData) {
            memset(data, 0, stride * getHeight());
        } else {
            fillRect(min, max, 0);
        }
    }

    void Image::load(const FT_Bitmap& ft_bitmap) {
        assert(getWidth() == ft_bitmap.width && getHeight() == ft_bitmap.rows && bitDepth == getFtBitdepth(ft_bitmap));
        if (min.x == 0 && min.y == 0) {
            assert(stride == static_cast<size_t>(ft_bitmap.pitch));
            memcpy(data, ft_bitmap.buffer, ft_bitmap.pitch * ft_bitmap.rows);
        } else if (min.x % 8 == 0) {
            assert(bitDepth == 1);
            for (size_t y = 0; y < ft_bitmap.rows; y++)
                memcpy(&data[(min.y + y) * stride + min.x / 8],
                       &ft_bitmap.buffer[y * ft_bitmap.pitch],
                       ft_bitmap.pitch);
        } else {
            for (size_t y = 0; y < ft_bitmap.rows; y++) {
                for (size_t x = 0; x < ft_bitmap.width; x++) {
                    setPixel<uint8_t>({x, y}, (ft_bitmap.buffer[y * ft_bitmap.pitch + x / 8] >> (7 - (x % 8))) & 1);
                }
            }
        }
    }

    size_t Image::getFtBitdepth(const FT_Bitmap& ft_bitmap) {
        switch (ft_bitmap.pixel_mode) {
            case FT_PIXEL_MODE_MONO:
                return 1;
            case FT_PIXEL_MODE_GRAY2:
                return 2;
            case FT_PIXEL_MODE_GRAY4:
                return 4;
            default:
                return 8;
        }
    }

    void Image::readData(png_structp png, png_bytep data, png_size_t length) {
        png_voidp a = png_get_io_ptr(png);
        ((std::istream*)a)->read((char*)data, length);
    }

    void Image::writeData(png_structp png, png_bytep data, png_size_t length) {
        png_voidp a = png_get_io_ptr(png);
        ((std::ostream*)a)->write((char*)data, length);
    }

    void Image::flushData(png_structp png) {
        png_voidp a = png_get_io_ptr(png);
        ((std::ostream*)a)->flush();
    }

    uint32_t Image::reduceBitDepth(uint32_t in, uint8_t in_bitDepth, uint8_t out_bitDepth) {
        uint32_t in_max = (1 << in_bitDepth) - 1,
                 out_max = (1 << out_bitDepth) - 1;
        return in * out_max / in_max;
    }

    Image::Image(const std::string& filepath, uint8_t _bitDepth) {
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

        png_set_read_fn(png, (png_voidp)&in_file, readData);

        png_set_sig_bytes(png, 8);

        png_read_info(png, info);

        min.x = 0;
        min.y = 0;
        max.x = png_get_image_width(png, info);
        max.y = png_get_image_height(png, info);
        uint32_t color_type = png_get_color_type(png, info);

        if (color_type == PNG_COLOR_TYPE_GRAY) {
            if (png_get_bit_depth(png, info) < 8) {
                png_set_expand_gray_1_2_4_to_8(png);
            }
        } else {
            if (png_get_valid(png, info, PNG_INFO_tRNS)) {
                png_set_tRNS_to_alpha(png);
            }
            if (color_type == PNG_COLOR_TYPE_PALETTE) {
                png_set_palette_to_rgb(png);
            } else if (color_type == PNG_COLOR_TYPE_RGB ||
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

        uint8_t png_bitDepth = png_get_bit_depth(png, info);
        size_t png_stride = (getWidth() * png_bitDepth + 7) / 8;

        std::unique_ptr<png_bytep> row_ptrs(new png_bytep[getHeight()]);
        std::unique_ptr<uint8_t> multi_channel_data(new uint8_t[getHeight() * png_stride * channels]);
        for (size_t i = 0; i < getHeight(); i++) {
            row_ptrs.get()[i] = (png_bytep)multi_channel_data.get() + i * png_stride * channels;
        }

        png_read_image(png, row_ptrs.get());

        png_destroy_read_struct(&png, &info, (png_infopp)0);
        in_file.close();

        bitDepth = (_bitDepth) ? _bitDepth : png_bitDepth;
        stride = (getWidth() * bitDepth + 7) / 8;
        data = new uint8_t[stride * getHeight()];
        isOwnerOfData = true;

        for (size_t y = 0; y < getHeight(); y++) {
            for (size_t x = 0; x < getWidth(); x++) {
                auto pixel = multi_channel_data.get()[y * png_stride * channels + x * png_bitDepth / 8 * channels];
                auto reducedPixel = reduceBitDepth(pixel, png_bitDepth, bitDepth);
                setPixel<uint16_t>({x, y}, reducedPixel);
            }
        }
    }

    template LLASSETGEN_API void Image::exportPng<uint32_t>(const std::string& filepath, uint32_t min, uint32_t max);
    template LLASSETGEN_API void Image::exportPng<uint16_t>(const std::string& filepath, uint16_t min, uint16_t max);
    template LLASSETGEN_API void Image::exportPng<uint8_t>(const std::string& filepath, uint8_t min, uint8_t max);
    template LLASSETGEN_API void Image::exportPng<float>(const std::string& filepath, float min, float max);
    template <typename pixelType>
    void Image::exportPng(const std::string& filepath, pixelType black, pixelType white) {
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

        if (setjmp(png_jmpbuf(png))) {
            std::cerr << "pnglib caused a longjump due to an error" << std::endl;
            std::cerr << "could not write file " << filepath;
            abort();
        }

        png_set_write_fn(png, (png_voidp)&out_file, writeData, flushData);

        png_set_IHDR(png,
            info,
            getWidth(),
            getHeight(),
            (bitDepth < 16) ? bitDepth : 16,
            PNG_COLOR_TYPE_GRAY,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_BASE,
            PNG_FILTER_TYPE_BASE);

        png_write_info(png, info);

        if (bitDepth > 8) {
            png_set_swap(png);
        }

        // TODO
        if (bitDepth >= 24) {
            std::unique_ptr<uint16_t[]> row(new uint16_t[getWidth()]);
            // possible 32 float or 32 or 24 bit int data
            // scale down to 16 bit int grayscale

            for (size_t y = 0; y < getHeight(); y++) {
                for (size_t x = 0; x < getWidth(); x++) {
                    auto value =
                        static_cast<float>(getPixel<pixelType>({x, y}) - black) / static_cast<float>(white - black);
                    row[x] = clamp(value, 0.0F, 1.0F) * std::numeric_limits<uint16_t>::max();
                }
                png_write_row(png, reinterpret_cast<png_bytep>(row.get()));
            }
        } else {
            for (size_t y = 0; y < getHeight(); y++) {
                png_write_row(png, reinterpret_cast<png_bytep>(&data[y * stride]));
            }
        }

        png_write_end(png, nullptr);

        png_destroy_write_struct(&png, &info);
        out_file.close();
    }
}
