#include <vector>
#include <iostream>
#include <fstream>
#include <cassert>
#include <cmath>
#include <memory>

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

    Image::Image(const FT_Bitmap &ft_bitmap, size_t padding) {
        min.x = 0;
        min.y = 0;
        max.x = ft_bitmap.width + 2 * padding;
        max.y = ft_bitmap.rows + 2 * padding;

        switch (ft_bitmap.pixel_mode) {
        case FT_PIXEL_MODE_MONO:
            bitDepth = 1;
            break;
        case FT_PIXEL_MODE_GRAY2:
            // We haven't found a single font using this format, however.
            bitDepth = 2;
            break;
        case FT_PIXEL_MODE_GRAY4:
            // We haven't found a single font using this format, however.
            bitDepth = 4;
            break;
        default:
            bitDepth = 8;
        }

        load(ft_bitmap, padding);
    }

    void Image::load(const FT_Bitmap &ft_bitmap, size_t padding) {
        assert(getWidth() == ft_bitmap.width + 2 * padding &&
               getHeight() == ft_bitmap.rows + 2 * padding);

        uint8_t ft_bitDepth;
        switch (ft_bitmap.pixel_mode) {
        case FT_PIXEL_MODE_MONO:
            ft_bitDepth = 1;
            break;
        case FT_PIXEL_MODE_GRAY2:
            // We haven't found a single font using this format, however.
            ft_bitDepth = 2;
            break;
        case FT_PIXEL_MODE_GRAY4:
            // We haven't found a single font using this format, however.
            ft_bitDepth = 4;
            break;
        default:
            ft_bitDepth = 8;
        }
        assert(ft_bitDepth == bitDepth);

        stride = size_t(std::ceil(float(getWidth() * bitDepth) / 8));
        size_t paddingBytes = (padding * bitDepth) / 8;

        /* because there are (padding % 8) padding bits, each byte from the original bitmap is
           split into (8 - padding % 8) left bits and (padding % 8) right bits. Thus every byte
           in the Image bitmap consists of the right bits of one original byte and the left bits
           of the byte that comes after it.
        */
        char rightBits = (padding * bitDepth) % 8;
        char leftBits = 8 - rightBits;

        data = std::shared_ptr<uint8_t>(new uint8_t[getHeight() * stride]);

        // y padding
        for (size_t y = 0; y < padding; y++) {
            for (size_t x = 0; x < stride; x++) {
                data.get()[y * stride + x] = 0xFF;
                data.get()[(getHeight() - y - 1) * stride + x] = 0xFF;
            }
        }

        for (size_t y = 0; y < ft_bitmap.rows; y++) {
            size_t padded_y = y + padding;

            // x padding
            for (size_t x = 0; x < paddingBytes; x++) {
                data.get()[padded_y * stride + x] = 0xFF;
                data.get()[(padded_y + 1) * stride - x - 1] = 0xFF;
            }

            uint8_t left = (0xFF >> leftBits) << leftBits;
            for (size_t x = 0; x < (stride - 2 * paddingBytes) - 1; x++) {
                size_t padded_x = x + paddingBytes;
                uint8_t val = ~(ft_bitmap.buffer[y * ft_bitmap.pitch + x]);
                data.get()[padded_y * stride + padded_x] = left | (val >> rightBits);
                left = val << leftBits;
            }
            size_t lastX = (stride - 2 * paddingBytes) - 1 + paddingBytes;
            data.get()[padded_y * stride + lastX] = left | (0xFF >> leftBits);
        }
    }

    Image::Image(const size_t width, const size_t height, const uint8_t _bitDepth) {
        assert(_bitDepth == 1 || _bitDepth == 2 || _bitDepth == 4 || _bitDepth == 8 || _bitDepth == 16 || _bitDepth == 24 || _bitDepth == 32);
        min.x = 0;
        min.y = 0;
        max.x = width;
        max.y = height;
        bitDepth = _bitDepth;
        stride = (size_t)std::ceil(float(getWidth() * (float(bitDepth) / 8.0f)));
        data = std::shared_ptr<uint8_t>(new uint8_t[getHeight() * stride]());
    }

    size_t Image::getWidth() const {
        return max.x - min.x;
    }

    size_t Image::getHeight() const {
        return max.y - min.y;
    }

    size_t Image::getBitDepth() const {
        return bitDepth;
    }

	Image Image::view(const Vec2<size_t> _min, const Vec2<size_t> _max) {
        return Image(_min, _max, stride, bitDepth, data);
    }

    Image::Image(const Vec2<size_t> _min, const Vec2<size_t> _max, const size_t _stride, const uint8_t _bitDepth, const std::shared_ptr<uint8_t>_data) {
        min = _min;
        max = _max;
        stride = _stride;
        bitDepth = _bitDepth;
        data = _data;
    }

    bool Image::isValid(const Vec2<size_t> pos) const {
        Vec2<size_t> offset = min + pos;
        return offset.x >= min.x && offset.x < max.x && offset.y >= min.y && offset.y < max.y;
    }

    template<typename pixelType>
    pixelType Image::getPixel(const Vec2<size_t> pos) const {
        assert(isValid(pos));
        Vec2<size_t> offset = min + pos;
        if (bitDepth <= 8) {
            uint8_t byte = data.get()[offset.y * stride + offset.x * bitDepth / 8];
            size_t bit_pos = offset.x % (8 / bitDepth);
            byte <<= bit_pos * bitDepth;
            byte >>= 8 - bitDepth;
            uint32_t casted = static_cast<uint32_t>(byte);
            return *reinterpret_cast<pixelType*>(&casted);
        } else {
            uint32_t return_data = 0;
            for (size_t byte_pos = 0; byte_pos < bitDepth / 8; ++byte_pos) {
                return_data <<= 8;
                return_data |= data.get()[offset.y * stride + offset.x * bitDepth / 8 + byte_pos];
            }
            return *reinterpret_cast<pixelType*>(&return_data);
        }
    }

    template LLASSETGEN_API float Image::getPixel<float>(const Vec2<size_t> pos) const;
    template LLASSETGEN_API uint32_t Image::getPixel<uint32_t>(const Vec2<size_t> pos) const;
    template LLASSETGEN_API uint16_t Image::getPixel<uint16_t>(const Vec2<size_t> pos) const;
    template LLASSETGEN_API uint8_t Image::getPixel<uint8_t>(const Vec2<size_t> pos) const;

    template<typename pixelType>
    void Image::setPixel(const Vec2<size_t> pos, pixelType in) const {
        assert(isValid(pos));
        Vec2<size_t> offset = min + pos;
        if (bitDepth <= 8) {
            uint8_t mask = 0xFF;
            uint8_t in_byte = static_cast<uint8_t>(*reinterpret_cast<uint32_t*>(&in));
            mask <<= 8 - bitDepth;
            mask >>= 8 - bitDepth;

            size_t bit_pos = offset.x % (8 / bitDepth);
            in_byte <<= 8 - bit_pos * bitDepth - bitDepth;
            mask <<= 8 - bit_pos * bitDepth - bitDepth;
            data.get()[offset.y * stride + offset.x * bitDepth / 8] = (data.get()[offset.y * stride + offset.x * bitDepth / 8] & ~mask) | in_byte;
        } else {
            uint32_t in_int = *reinterpret_cast<uint32_t*>(&in);
            for (int byte_pos = bitDepth / 8 - 1; byte_pos >= 0; byte_pos--) {
                data.get()[offset.y * stride + offset.x * bitDepth / 8 + byte_pos] = static_cast<uint8_t>(in_int);
                in_int >>= 8;
            }
        }
    }

    template LLASSETGEN_API void Image::setPixel<float>(const Vec2<size_t> pos, const float in) const;
    template LLASSETGEN_API void Image::setPixel<uint32_t>(const Vec2<size_t> pos, const uint32_t in) const;
    template LLASSETGEN_API void Image::setPixel<uint16_t>(const Vec2<size_t> pos, const uint16_t in) const;
    template LLASSETGEN_API void Image::setPixel<uint8_t>(const Vec2<size_t> pos, const uint8_t in) const;

    template <typename pixelType>
    void Image::exportPng(const std::string &filepath, pixelType black, pixelType white) {
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

        if (bitDepth >= 24) {
            std::unique_ptr<uint16_t[]> row(new uint16_t[getWidth()]);
            // possible 32 float or 32 or 24 bit int data
            // scale down to 16 bit int grayscale

            for (size_t y = 0; y < getHeight(); y++) {
                for (size_t x = 0; x < getWidth(); x++) {
                    row[x] = clamp(static_cast<float>(getPixel<pixelType>({x, y}) - black) / (white - black), 0.0F, 1.0F) * std::numeric_limits<uint16_t>::max();
                }
                png_write_row(png, reinterpret_cast<png_bytep>(row.get()));
            }
        } else {
            for (size_t y = 0; y < getHeight(); y++) {
                png_write_row(png, reinterpret_cast<png_bytep>(&data.get()[y * stride]));
            }
        }

        png_write_end(png, nullptr);

        png_destroy_write_struct(&png, &info);
        out_file.close();
    }

    template LLASSETGEN_API void Image::exportPng<uint32_t>(const std::string &filepath, uint32_t min, uint32_t max);
    template LLASSETGEN_API void Image::exportPng<uint16_t>(const std::string &filepath, uint16_t min, uint16_t max);
    template LLASSETGEN_API void Image::exportPng<uint8_t>(const std::string &filepath, uint8_t min, uint8_t max);
    template LLASSETGEN_API void Image::exportPng<float>(const std::string &filepath, float min, float max);

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

    Image::Image(const std::string &filepath, uint8_t _bitDepth) {
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
            } if (color_type == PNG_COLOR_TYPE_PALETTE) {
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
        size_t png_stride = size_t(ceil(float(getWidth()) * (float(png_bitDepth) / 8.0f)));

        std::unique_ptr<png_bytep> row_ptrs(new png_bytep[getHeight()]);
        std::unique_ptr<uint8_t> multi_channel_data(new uint8_t[getHeight() * png_stride * channels]);
        for (size_t i = 0; i < getHeight(); i++) {
            row_ptrs.get()[i] = (png_bytep)multi_channel_data.get() + i * png_stride * channels;
        }

        png_read_image(png, row_ptrs.get());

        png_destroy_read_struct(&png, &info, (png_infopp)0);
        in_file.close();

        bitDepth = (_bitDepth) ? _bitDepth : png_bitDepth;
        stride = size_t(ceil(float(getWidth()) * (float(bitDepth) / 8.0f)));

        data = std::shared_ptr<uint8_t>(new uint8_t[getHeight() * stride]);
        for (size_t y = 0; y < getHeight(); y++) {
            for (size_t x = 0; x < getWidth(); x++) {
                setPixel<uint16_t>({x, y}, reduceBitDepth(multi_channel_data.get()[y * png_stride * channels + x * png_bitDepth / 8 * channels], png_bitDepth, bitDepth));
            }
        }
    }

    uint16_t Image::reduceBitDepth(uint16_t in, uint8_t in_bitDepth, uint8_t out_bitDepth) {
        int in_max = (1 << in_bitDepth) - 1;
        int out_max = (1 << out_bitDepth) - 1;
        uint16_t reduced = static_cast<uint16_t>(round(float(in) / float(in_max) * float(out_max)));
        return reduced;
    }
}
