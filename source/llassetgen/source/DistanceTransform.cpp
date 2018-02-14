#include <iostream>
#include <cassert>
#include <cmath>

#include <llassetgen/DistanceTransform.h>

namespace llassetgen {
    template<typename PixelType, bool flipped, bool invalidBounds>
    PixelType DistanceTransform::getPixel(PositionType pos) {
        const Image& image = std::is_same<PixelType, InputType>::value ? input : output;
        if(invalidBounds && !image.is_valid(pos))
            return 0;
        if(flipped)
            std::swap(pos.x, pos.y);
        PixelType value = image.getPixel<PixelType>(pos);
        return std::is_same<PixelType, InputType>::value ? (value >= std::numeric_limits<InputType>::max()/2) : value; // TODO
    }

    template<typename PixelType, bool flipped>
    void DistanceTransform::setPixel(PositionType pos, PixelType value) {
        const Image& image = std::is_same<PixelType, InputType>::value ? input : output;
        if(flipped)
            std::swap(pos.x, pos.y);
        image.setPixel<PixelType>(pos, value);
    }

    DistanceTransform::DistanceTransform(const Image& _input, const Image& _output) :input(_input), output(_output) {
        assert(input.get_width() == output.get_width() && input.get_height() == output.get_height());
    }

    /*DeadReckoning::PositionType& DeadReckoning::posAt(PositionType pos) {
        assert(pos.x < width && pos.y < height && posBuffer.get());
        return posBuffer[pos.y * width + pos.x];
    }

    void DeadReckoning::transformAt(PositionType pos, PositionType target, OutputType distance) {
        target += pos;
        if (outputAtClamped(target) + distance < outputAt(pos)) {
            posAt(pos) = target = posAt(target);
            outputAt(pos) = std::sqrt(square(pos.x - target.x) + square(pos.y - target.y));
        }
    }

    void DeadReckoning::transform() {
        assert(width > 0 && height > 0);
        output.reset(new OutputType[width * height]);
        posBuffer.reset(new PositionType[width * height]);

        for (DimensionType y = 0; y < height; ++y)
            for (DimensionType x = 0; x < width; ++x) {
                PositionType pos = {x, y};
                bool center = inputAt(pos);
                posAt(pos) = pos;
                outputAt(pos) =
                    (center && (inputAtClamped({x - 1, y}) != center || inputAtClamped({x + 1, y}) != center ||
                                inputAtClamped({x, y - 1}) != center || inputAtClamped({x, y + 1}) != center))
                        ? 0
                        : std::numeric_limits<OutputType>::infinity();
            }

        const OutputType distance[] = {std::sqrt(2.0F), 1.0F, std::sqrt(2.0F), 1.0F};
        const PositionType target[] = {{-1, -1}, {0, -1}, {+1, -1}, {-1, 0}};

        for (DimensionType y = 0; y < height; ++y)
            for (DimensionType x = 0; x < width; ++x)
                for (DimensionType i = 0; i < 4; ++i)
                    transformAt({x, y}, target[i], distance[i]);

        for (DimensionType y = 0; y < height; ++y)
            for (DimensionType x = 0; x < width; ++x)
                for (DimensionType i = 0; i < 4; ++i)
                    transformAt({width - x - 1, height - y - 1}, -(target[3 - i]), distance[3 - i]);

        for (DimensionType y = 0; y < height; ++y)
            for (DimensionType x = 0; x < width; ++x)
                if (inputAt({x, y}))
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
            DimensionType end = (next) ? j : j-1;
            if(fill)
                for(DimensionType i = begin; i < end; ++i) {
                    if(begin == 0)
                        outputAt(offset+i*pitch) = square(end-i);
                    else
                        outputAt(offset+i*pitch) = square((i < (end+begin)/2) ? i-begin+1 : end-i);
                }
            prev = next;
            begin = end+1;
            outputAt(offset+end*pitch) = 0;
        }
        if(fill)
            for(DimensionType i = begin; i < length; ++i) {
                if(begin == 0)
                    outputAt(offset+i*pitch) = std::numeric_limits<OutputType>::max();
                else if(!prev)
                    outputAt(offset+i*pitch) = square(i-begin+1);
                else
                    outputAt(offset+i*pitch) = square((i < (length+begin)/2) ? i-begin+1 : length-i);
            }
        if(prev)
            outputAt(offset+(length-1)*pitch) = 0;
    }

    void ParabolaEnvelope::transformLine(DimensionType offset, DimensionType pitch, DimensionType length) {
        parabolas[0].apex = 0;
        parabolas[0].begin = -std::numeric_limits<OutputType>::infinity();
        parabolas[0].value = outputAt(offset + 0 * pitch);
        parabolas[1].begin = +std::numeric_limits<OutputType>::infinity();
        for (DimensionType parabola = 0, i = 1; i < length; ++i) {
            OutputType begin;
            do {
                DimensionType apex = parabolas[parabola].apex;
                begin = (outputAt(offset + i * pitch) + square(i) - (parabolas[parabola].value + square(apex))) /
                        (2 * (i - apex));
            } while (begin <= parabolas[parabola--].begin);
            parabola += 2;
            parabolas[parabola].apex = i;
            parabolas[parabola].begin = begin;
            parabolas[parabola].value = outputAt(offset + i * pitch);
            parabolas[parabola + 1].begin = std::numeric_limits<OutputType>::infinity();
        }
        for (DimensionType parabola = 0, i = 0; i < length; ++i) {
            while (parabolas[++parabola].begin < i)
                ;
            --parabola;
            outputAt(offset+i*pitch) = std::sqrt(parabolas[parabola].value+square(i-parabolas[parabola].apex)) * (inputAt(offset+i*pitch) ? -1 : 1);
        }
    }

    void ParabolaEnvelope::transform() {
        assert(width > 0 && height > 0);
        output.reset(new OutputType[width * height]);
        DimensionType length = std::max(width, height);
        parabolas.reset(new Parabola[length + 1]);
        lineBuffer.reset(new OutputType[length]);

        for(DimensionType y = 0; y < height; ++y)
            edgeDetection<true>(y*width, 1, width);

        for(DimensionType x = 0; x < width; ++x) {
            edgeDetection<false>(x, width, height);
            transformLine(x, width, height);
        }
    }*/
}
