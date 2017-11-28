#include <algorithm>
#include <limits>

#include <ft2build.h>
#include <freetype/freetype.h>
#include <png.h>

#include <llassetgen/DistanceTransform.h>



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

    bool DistanceTransform::inputAt(DimensionType offset) {
        assert(offset < width * height && input.get());
        return (input[offset/8] >> (offset%8)) & 1;
    }

    bool DistanceTransform::inputAt(PositionType pos) {
        assert(pos.x < width && pos.y < height);
        return inputAt(pos.y*width+pos.x);
    }

    bool DistanceTransform::inputAtClamped(PositionType pos) {
        if(static_cast<int>(pos.x) < 0 || static_cast<int>(pos.y) < 0 || pos.x >= width || pos.y >= height)
            return 0;
        return inputAt(pos);
    }

    void DistanceTransform::inputAt(PositionType pos, bool bit) {
        assert(pos.x < width && pos.y < height && input.get());
        DimensionType i = pos.y*width+pos.x;
        InputType mask = 1 << (i%8);
        i /= 8;
        if(bit)
            input[i] |= mask;
        else
            input[i] &= ~mask;
    }

    DistanceTransform::OutputType& DistanceTransform::outputAt(DimensionType offset) {
        assert(offset < width * height && output.get());
        return output[offset];
    }

    DistanceTransform::OutputType& DistanceTransform::outputAt(PositionType pos) {
        assert(pos.x < width && pos.y < height && output.get());
        return output[pos.y*width+pos.x];
    }

    DistanceTransform::OutputType DistanceTransform::outputAtClamped(PositionType pos) {
        assert(output.get());
        if(static_cast<int>(pos.x) < 0 || static_cast<int>(pos.y) < 0 || pos.x >= width || pos.y >= height)
            return std::numeric_limits<OutputType>::infinity();
        return output[pos.y*width+pos.x];
    }

    void DistanceTransform::importFreeTypeBitmap(FT_Bitmap* bitmap, DimensionType padding) {
        assert(bitmap);
        resetInput(bitmap->width + padding*2, bitmap->rows + padding*2, true);
        for(DimensionType y = 0; y < bitmap->rows; ++y)
            for(DimensionType x = 0; x < bitmap->width; ++x)
                inputAt({padding+x, padding+y}, (bitmap->buffer[y*bitmap->pitch+(x/8)] >> (7-x%8)) & 1);
    }

    void DistanceTransform::importPng(std::string path) {
        FILE* file = fopen(path.c_str(), "r");
        assert(file);
        int bitDepth = 0, colorType = 0;
        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
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
            png_get_IHDR(png, pngInfo, &width, &height, &bitDepth, &colorType, nullptr, nullptr, nullptr);
            assert(colorType == PNG_COLOR_TYPE_GRAY);
            assert(bitDepth == 8);
            DimensionType index = 0;
            InputType byte = 0, bit = 0;
            resetInput(width, height, false);
            std::unique_ptr<InputType[]> rowBuffer(new InputType[width]);
            for(DimensionType y = 0; y < height; ++y) {
                png_read_row(png, reinterpret_cast<png_bytep>(rowBuffer.get()), nullptr);
                for(DimensionType x = 0; x < width; ++x) {
                    byte |= (rowBuffer[x] >= std::numeric_limits<InputType>::max()/2 ? 1 : 0) << bit;
                    if(++bit >= 8) {
                        input[index++] = byte;
                        byte = bit = 0;
                    }
                }
            }
        }
        png_read_end(png, nullptr);
        png_destroy_read_struct(&png, &pngInfo, nullptr);
        fclose(file);
    }

    template<typename PixelType>
    void DistanceTransform::exportPngInternal(png_struct* png, OutputType blackDistance, OutputType whiteDistance) {
        std::unique_ptr<PixelType[]> rowBuffer(new PixelType[width]);
        for(DimensionType y = 0; y < height; ++y) {
            for(DimensionType x = 0; x < width; ++x)
                rowBuffer[x] = clamp((outputAt({x, y})-blackDistance) / (whiteDistance-blackDistance), 0.0F, 1.0F) * std::numeric_limits<PixelType>::max();
            png_write_row(png, reinterpret_cast<png_bytep>(rowBuffer.get()));
        }
    }

    void DistanceTransform::exportPng(std::string path, OutputType blackDistance, OutputType whiteDistance, DimensionType bitDepth) {
        assert(width > 0 && height > 0 && output.get());
        FILE* file = fopen(path.c_str(), "w");
        assert(file);
        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
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
        png_write_end(png, nullptr);
        png_destroy_write_struct(&png, &pngInfo);
        fclose(file);
    }



    DeadReckoning::PositionType& DeadReckoning::posAt(PositionType pos) {
        assert(pos.x < width && pos.y < height && posBuffer.get());
        return posBuffer[pos.y*width+pos.x];
    }

    void DeadReckoning::transformAt(PositionType pos, PositionType target, OutputType distance) {
        target += pos;
        if(outputAtClamped(target)+distance < outputAt(pos)) {
            posAt(pos) = target = posAt(target);
            outputAt(pos) = std::sqrt(square(pos.x-target.x)+square(pos.y-target.y));
        }
    }

    void DeadReckoning::transform() {
        assert(width > 0 && height > 0);
        output.reset(new OutputType[width * height]);
        posBuffer.reset(new PositionType[width * height]);

        for(DimensionType y = 0; y < height; ++y)
            for(DimensionType x = 0; x < width; ++x) {
                PositionType pos = {x, y};
                bool center = inputAt(pos);
                posAt(pos) = pos;
                outputAt(pos) = (center && (
                                 inputAtClamped({x-1, y}) != center || inputAtClamped({x+1, y}) != center ||
                                 inputAtClamped({x, y-1}) != center || inputAtClamped({x, y+1}) != center))
                                ? 0 : std::numeric_limits<OutputType>::infinity();
            }

        const OutputType distance[] = {
            std::sqrt(2.0F), 1.0F, std::sqrt(2.0F), 1.0F
        };
        const PositionType target[] = {
            {-1, -1}, { 0, -1}, {+1, -1}, {-1,  0}
        };

        for(DimensionType y = 0; y < height; ++y)
            for(DimensionType x = 0; x < width; ++x)
                for(DimensionType i = 0; i < 4; ++i)
                    transformAt({x, y}, target[i], distance[i]);

        for(DimensionType y = 0; y < height; ++y)
            for(DimensionType x = 0; x < width; ++x)
                for(DimensionType i = 0; i < 4; ++i)
                    transformAt({width-x-1, height-y-1}, -(target[3-i]), distance[3-i]);

        for(DimensionType y = 0; y < height; ++y)
            for(DimensionType x = 0; x < width; ++x)
                if(inputAt({x, y}))
                    outputAt({x, y}) *= -1;
    }



    template<bool fill>
    void ParabolaEnvelope::edgeDetection(DimensionType offset, DimensionType pitch, DimensionType length) {
        InputType prev = 0;
        DimensionType begin = 0;
        for(DimensionType j = 0; j < length; ++j) {
            InputType next = inputAt(offset+j*pitch);
            if(next == prev)
                continue;
            DimensionType end = (next) ? j-1 : j;
            if(fill)
                for(DimensionType i = begin; i < end; ++i) {
                    if(begin == 0 && !prev)
                        outputAt(offset+i*pitch) = square(end-i);
                    else
                        outputAt(offset+i*pitch) = square((i < (end+begin+1)/2) ? i-begin : end-i);
                }
            prev = next;
            begin = end;
            outputAt(offset+end*pitch) = 0;
        }
        if(fill)
            for(DimensionType i = begin; i < length; ++i) {
                if(begin == 0 && !prev)
                    outputAt(offset+i*pitch) = 1E20F;
                else if(!prev)
                    outputAt(offset+i*pitch) = square(i-begin);
                else
                    outputAt(offset+i*pitch) = square((i < (length+begin+1)/2) ? i-begin : length-i);
            }
        if(prev)
            outputAt(offset+(length-1)*pitch) = 0;
    }

    void ParabolaEnvelope::transformLine(DimensionType offset, DimensionType pitch, DimensionType length) {
        parabolas[0].apex = 0;
        parabolas[0].begin = -std::numeric_limits<OutputType>::infinity();
        parabolas[0].value = outputAt(offset+0*pitch);
        parabolas[1].begin = +std::numeric_limits<OutputType>::infinity();
        for(DimensionType parabola = 0, i = 1; i < length; ++i) {
            OutputType begin;
            do {
                DimensionType apex = parabolas[parabola].apex;
                begin = (outputAt(offset+i*pitch)+square(i) - (parabolas[parabola].value+square(apex))) / (2 * (i - apex));
            } while(begin <= parabolas[parabola--].begin);
            parabola += 2;
            parabolas[parabola].apex = i;
            parabolas[parabola].begin = begin;
            parabolas[parabola].value = outputAt(offset+i*pitch);
            parabolas[parabola+1].begin = std::numeric_limits<OutputType>::infinity();
        }
        for(DimensionType parabola = 0, i = 0; i < length; ++i) {
            while(parabolas[++parabola].begin < i);
            --parabola;
            outputAt(offset+i*pitch) = std::sqrt(parabolas[parabola].value+square(i-parabolas[parabola].apex)) * (inputAt(offset+i*pitch) ? -1 : 1);
        }
    }

    void ParabolaEnvelope::transform() {
        assert(width > 0 && height > 0);
        output.reset(new OutputType[width * height]);
        DimensionType length = std::max(width, height);
        parabolas.reset(new Parabola[length+1]);
        lineBuffer.reset(new OutputType[length]);

        for(DimensionType y = 0; y < height; ++y)
            edgeDetection<true>(y*width, 1, width);

        for(DimensionType x = 0; x < width; ++x) {
            edgeDetection<false>(x, width, height);
            transformLine(x, width, height);
        }
    }
}
