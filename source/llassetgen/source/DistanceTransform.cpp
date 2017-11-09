#include <algorithm>
#include <limits>

#include <ft2build.h>
#include <freetype/freetype.h>
#include <png.h>

#include <llassetgen/llassetgen.h>



template<typename T>
T square(T value) {
    return value * value;
}

template<typename T>
T clamp(T val, T min, T max) {
    return std::max(min, std::min(max, val));
}

namespace llassetgen {
    void DistanceTransform::resetInput(DimensionType _width, DimensionType _height, bool clear) {
        assert(_width > 0 && _height > 0);
        width = _width;
        height = _height;
        DimensionType size = (width * height + 7) / 8;
        input.reset(new InputType[size]);
        if(clear)
            memset(input.get(), 0, size);
    }

    DistanceTransform::InputType DistanceTransform::inputAt(DimensionType x, DimensionType y) {
        assert(x < width && y < height && input.get());
        DimensionType i = y*width+x;
        return (input[i/8] >> (i%8)) & 1;
    }

    void DistanceTransform::inputAt(DimensionType x, DimensionType y, InputType bit) {
        assert(x < width && y < height && input.get());
        DimensionType i = y*width+x;
        InputType mask = 1 << (i%8);
        i /= 8;
        if(bit)
            input[i] |= mask;
        else
            input[i] &= ~mask;
    }

    DistanceTransform::OutputType& DistanceTransform::outputAt(DimensionType x, DimensionType y) {
        assert(x < width && y < height && output.get());
        return output[y*width+x];
    }

    void DistanceTransform::importFreeTypeBitmap(FT_Bitmap* bitmap, DimensionType padding) {
        resetInput(bitmap->width + padding*2, bitmap->rows + padding*2, true);
        for(DimensionType y = 0; y < bitmap->rows; ++y)
            for(DimensionType x = 0; x < bitmap->width; ++x)
                inputAt(padding+x, padding+y, (bitmap->buffer[y*bitmap->pitch+(x/8)] >> (7-x%8)) & 1);
    }

    void DistanceTransform::importPng(std::string path) {
        FILE* file = fopen(path.c_str(), "r");
        assert(file);
        int bitDepth = 0, colorType = 0;
        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop pngInfo = png_create_info_struct(png);
        if(setjmp(png_jmpbuf(png)))
            assert(false);
        else {
            png_init_io(png, file);
            png_read_info(png, pngInfo);
            png_set_strip_16(png);
            if(colorType == PNG_COLOR_TYPE_PALETTE)
                png_set_palette_to_rgb(png);
            if(colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
                png_set_expand_gray_1_2_4_to_8(png);
            assert(png_set_interlace_handling(png) == 1);
            png_read_update_info(png, pngInfo);
            png_get_IHDR(png, pngInfo, &width, &height, &bitDepth, &colorType, NULL, NULL, NULL);
            assert(colorType == PNG_COLOR_TYPE_GRAY);
            assert(bitDepth == 8);
            DimensionType index = 0;
            InputType byte = 0, bit = 0;
            resetInput(width, height, false);
            std::unique_ptr<InputType[]> rowBuffer(new InputType[width]);
            for(DimensionType y = 0; y < height; ++y) {
                png_read_row(png, reinterpret_cast<png_bytep>(rowBuffer.get()), NULL);
                for(DimensionType x = 0; x < width; ++x) {
                    byte |= (rowBuffer[x] >= std::numeric_limits<InputType>::max()/2 ? 1 : 0) << bit;
                    if(++bit >= 8) {
                        input[index++] = byte;
                        byte = bit = 0;
                    }
                }
            }
        }
        png_read_end(png, NULL);
        png_destroy_read_struct(&png, &pngInfo, NULL);
        fclose(file);
    }

    template<typename PixelType>
    void DistanceTransform::exportPngInternal(png_struct* png, OutputType blackDistance, OutputType whiteDistance) {
        std::unique_ptr<PixelType[]> rowBuffer(new PixelType[width]);
        for(DimensionType y = 0; y < height; ++y) {
            for(DimensionType x = 0; x < width; ++x)
                rowBuffer[x] = clamp((outputAt(x, y)-blackDistance) / (whiteDistance-blackDistance), 0.0F, 1.0F) * std::numeric_limits<PixelType>::max();
            png_write_row(png, reinterpret_cast<png_bytep>(rowBuffer.get()));
        }
    }

    void DistanceTransform::exportPng(std::string path, OutputType blackDistance, OutputType whiteDistance, DimensionType bitDepth) {
        assert(width > 0 && height > 0 && output.get());
        FILE* file = fopen(path.c_str(), "w");
        assert(file);
        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop pngInfo = png_create_info_struct(png);
        if(setjmp(png_jmpbuf(png)))
            assert(false);
        else {
            png_init_io(png, file);
            png_set_IHDR(png, pngInfo, width, height,
                         bitDepth, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                         PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
            png_write_info(png, pngInfo);
            png_set_swap(png);
            switch(bitDepth) {
                case 8:
                    exportPngInternal<unsigned char>(png, blackDistance, whiteDistance);
                    break;
                case 16:
                    exportPngInternal<unsigned short>(png, blackDistance, whiteDistance);
                    break;
                default:
                    assert(false);
            }
        }
        png_write_end(png, NULL);
        png_destroy_write_struct(&png, &pngInfo);
        fclose(file);
    }
}
