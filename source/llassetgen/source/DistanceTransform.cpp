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



    DeadReckoning::PositionType& DeadReckoning::posAt(PositionType pos) {
        assert(pos.x < input.get_width() && pos.y < input.get_height() && posBuffer.get());
        return posBuffer[pos.y * input.get_width() + pos.x];
    }

    void DeadReckoning::transformAt(PositionType pos, PositionType target, OutputType distance) {
        target += pos;
        if (output.is_valid(target) && getPixel<OutputType>(target) + distance < getPixel<OutputType>(pos)) {
            posAt(pos) = target = posAt(target);
            setPixel<OutputType>(pos, std::sqrt(square(pos.x - target.x) + square(pos.y - target.y)));
        }
    }

    void DeadReckoning::transform() {
        assert(input.get_width() > 0 && input.get_height() > 0);
        posBuffer.reset(new PositionType[input.get_width() * input.get_height()]);

        for (DimensionType y = 0; y < input.get_height(); ++y)
            for (DimensionType x = 0; x < input.get_width(); ++x) {
                PositionType pos = {x, y};
                bool center = getPixel<InputType, false>(pos);
                posAt(pos) = pos;
                setPixel<OutputType>(pos,
                    (center && (getPixel<InputType, false, true>({x - 1, y}) != center || getPixel<InputType, false, true>({x + 1, y}) != center ||
                                getPixel<InputType, false, true>({x, y - 1}) != center || getPixel<InputType, false, true>({x, y + 1}) != center))
                    ? 0
                    : std::numeric_limits<OutputType>::infinity()
                );
            }

        const OutputType distance[] = {std::sqrt(2.0F), 1.0F, std::sqrt(2.0F), 1.0F};
        const PositionType target[] = {
            {static_cast<DimensionType>(-1), static_cast<DimensionType>(-1)},
            {static_cast<DimensionType>( 0), static_cast<DimensionType>(-1)},
            {static_cast<DimensionType>(+1), static_cast<DimensionType>(-1)},
            {static_cast<DimensionType>(-1), static_cast<DimensionType>( 0)}
        };

        for (DimensionType y = 0; y < input.get_height(); ++y)
            for (DimensionType x = 0; x < input.get_width(); ++x)
                for (DimensionType i = 0; i < 4; ++i)
                    transformAt({x, y}, target[i], distance[i]);

        for (DimensionType y = 0; y < input.get_height(); ++y)
            for (DimensionType x = 0; x < input.get_width(); ++x)
                for (DimensionType i = 0; i < 4; ++i)
                    transformAt({input.get_width() - x - 1, input.get_height() - y - 1}, -(target[3 - i]), distance[3 - i]);

        for (DimensionType y = 0; y < input.get_height(); ++y)
            for (DimensionType x = 0; x < input.get_width(); ++x) {
                Vec2<DimensionType> pos(x, y);
                if (getPixel<InputType, false>(pos))
                    setPixel<OutputType>(pos, -getPixel<OutputType>(pos));
            }
    }



    template<bool flipped, bool fill>
    void ParabolaEnvelope::edgeDetection(DimensionType offset, DimensionType length) {
        InputType prev = 0;
        DimensionType begin = 0;
        for(DimensionType j = 0; j < length; ++j) {
            InputType next = getPixel<InputType, flipped>({j, offset});
            if(next == prev)
                continue;
            DimensionType end = (next) ? j : j-1;
            if(fill)
                for(DimensionType i = begin; i < end; ++i)
                    setPixel<OutputType, flipped>({i, offset}, square((begin == 0)
                        ? (end-i)
                        : ((i < (end+begin)/2) ? i-begin+1 : end-i)
                    ));
            prev = next;
            begin = end+1;
            setPixel<OutputType, flipped>({end, offset}, 0);
        }
        if(fill)
            for(DimensionType i = begin; i < length; ++i)
                setPixel<OutputType, flipped>({i, offset}, (begin == 0)
                    ? std::numeric_limits<OutputType>::max()
                    : square((!prev)
                        ? (i-begin+1)
                        : ((i < (length+begin)/2) ? i-begin+1 : length-i)
                    )
                );
        if(prev)
            setPixel<OutputType, flipped>({length-1, offset}, 0);
    }

    template<bool flipped>
    void ParabolaEnvelope::transformLine(DimensionType offset, DimensionType length) {
        parabolas[0].apex = 0;
        parabolas[0].begin = -std::numeric_limits<OutputType>::infinity();
        parabolas[0].value = getPixel<OutputType, flipped>({0, offset});
        parabolas[1].begin = +std::numeric_limits<OutputType>::infinity();
        for (DimensionType parabola = 0, i = 1; i < length; ++i) {
            OutputType begin;
            do {
                DimensionType apex = parabolas[parabola].apex;
                begin = (getPixel<OutputType, flipped>({i, offset}) + square(i) - (parabolas[parabola].value + square(apex))) / (2 * (i - apex));
            } while (begin <= parabolas[parabola--].begin);
            parabola += 2;
            parabolas[parabola].apex = i;
            parabolas[parabola].begin = begin;
            parabolas[parabola].value = getPixel<OutputType, flipped>({i, offset});
            parabolas[parabola + 1].begin = std::numeric_limits<OutputType>::infinity();
        }
        for (DimensionType parabola = 0, i = 0; i < length; ++i) {
            while (parabolas[++parabola].begin < i);
            --parabola;
            setPixel<OutputType, flipped>({i, offset}, std::sqrt(parabolas[parabola].value+square(i-parabolas[parabola].apex)) * (getPixel<InputType, flipped>({i, offset}) ? -1 : 1));
        }
    }

    void ParabolaEnvelope::transform() {
        assert(input.get_width() > 0 && input.get_height() > 0);
        DimensionType length = std::max(input.get_width(), input.get_height());
        parabolas.reset(new Parabola[length + 1]);
        lineBuffer.reset(new OutputType[length]);

        for(DimensionType y = 0; y < input.get_height(); ++y)
            edgeDetection<false, true>(y, input.get_width());

        for(DimensionType x = 0; x < input.get_width(); ++x) {
            edgeDetection<true, false>(x, input.get_height());
            transformLine<true>(x, input.get_height());
        }
    }
}
