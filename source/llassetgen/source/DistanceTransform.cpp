#include <cassert>
#include <cmath>

#include <llassetgen/DistanceTransform.h>

namespace llassetgen {
    template <typename PixelType, bool flipped, bool invalidBounds>
    PixelType DistanceTransform::getPixel(PositionType pos) {
        const Image& image = std::is_same<PixelType, InputType>::value ? input : output;
        if (invalidBounds && !image.isValid(pos))
            return 0;
        if (flipped)
            std::swap(pos.x, pos.y);
        return image.getPixel<PixelType>(pos);
    }

    template <typename PixelType, bool flipped>
    void DistanceTransform::setPixel(PositionType pos, PixelType value) {
        const Image& image = std::is_same<PixelType, InputType>::value ? input : output;
        if (flipped)
            std::swap(pos.x, pos.y);
        image.setPixel<PixelType>(pos, value);
    }

    DistanceTransform::DistanceTransform(const Image& _input, const Image& _output) : input(_input), output(_output) {
        assert(input.getWidth() == output.getWidth() &&
               input.getHeight() == output.getHeight() &&
               input.getBitDepth() == 1);
    }

    DeadReckoning::PositionType& DeadReckoning::posAt(PositionType pos) {
        assert(pos.x < input.getWidth() && pos.y < input.getHeight() && posBuffer.get());
        return posBuffer[pos.y * input.getWidth() + pos.x];
    }

    void DeadReckoning::transformAt(PositionType pos, PositionType target, OutputType distance) {
        target += pos;
        if (output.isValid(target) && getPixel<OutputType>(target) + distance < getPixel<OutputType>(pos)) {
            posAt(pos) = target = posAt(target);
            setPixel<OutputType>(pos, std::sqrt(square(pos.x - target.x) + square(pos.y - target.y)));
        }
    }

    void DeadReckoning::transform() {
        assert(input.getWidth() > 0 && input.getHeight() > 0);
        posBuffer.reset(new PositionType[input.getWidth() * input.getHeight()]);

        for (DimensionType y = 0; y < input.getHeight(); ++y)
            for (DimensionType x = 0; x < input.getWidth(); ++x) {
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

        for (DimensionType y = 0; y < input.getHeight(); ++y)
            for (DimensionType x = 0; x < input.getWidth(); ++x)
                for (DimensionType i = 0; i < 4; ++i)
                    transformAt({x, y}, target[i], distance[i]);

        for (DimensionType y = 0; y < input.getHeight(); ++y)
            for (DimensionType x = 0; x < input.getWidth(); ++x)
                for (DimensionType i = 0; i < 4; ++i)
                    transformAt({input.getWidth() - x - 1, input.getHeight() - y - 1}, -(target[3 - i]), distance[3 - i]);

        for (DimensionType y = 0; y < input.getHeight(); ++y)
            for (DimensionType x = 0; x < input.getWidth(); ++x) {
                Vec2<DimensionType> pos(x, y);
                if (getPixel<InputType, false>(pos))
                    setPixel<OutputType>(pos, -getPixel<OutputType>(pos));
            }
    }

    template <bool flipped, bool fill>
    void ParabolaEnvelope::edgeDetection(DimensionType offset, DimensionType length) {
        InputType prev = 0;
        DimensionType begin = 0;
        for (DimensionType j = 0; j < length; ++j) {
            InputType next = getPixel<InputType, flipped>({j, offset});
            if (next == prev)
                continue;
            DimensionType end = (next) ? j : j - 1;
            if (fill)
                for (DimensionType i = begin; i < end; ++i)
                    setPixel<OutputType, flipped>(
                        {i, offset},
                        square(begin == 0
                            ? (end - i) // Falling slope
                            : ((i < (end + begin) / 2) ? i - begin + 1 : end - i) // Rising and falling slope
                        )
                    );
            prev = next;
            begin = end + 1;
            setPixel<OutputType, flipped>({end, offset}, 0);  // Mark edge
        }
        if (fill)
            for (DimensionType i = begin; i < length; ++i)
                setPixel<OutputType, flipped>(
                    {i, offset},
                    (begin == 0)
                        ? std::numeric_limits<OutputType>::max() // Empty
                        : square(prev
                            ? ((i < (length - 1 + begin) / 2) ? i - begin + 1 : length - 1 - i) // Rising and falling slope
                            : (i - begin + 1) // Rising slope
                        )
                );
        if (prev) setPixel<OutputType, flipped>({length - 1, offset}, 0); // Mark edge
    }

    template <bool flipped>
    void ParabolaEnvelope::transformLine(DimensionType offset, DimensionType length) {
        parabolas[0].apex = 0;
        parabolas[0].begin = -std::numeric_limits<OutputType>::infinity();
        parabolas[0].value = getPixel<OutputType, flipped>({0, offset});
        parabolas[1].begin = +std::numeric_limits<OutputType>::infinity();
        for (DimensionType parabola = 0, i = 1; i < length; ++i) {
            OutputType begin;
            do {
                DimensionType apex = parabolas[parabola].apex;
                auto pixel = getPixel<OutputType, flipped>({i, offset});
                begin = (pixel + square(i) - (parabolas[parabola].value + square(apex))) / (2 * (i - apex));
            } while (begin <= parabolas[parabola--].begin);
            parabola += 2;
            parabolas[parabola].apex = i;
            parabolas[parabola].begin = begin;
            parabolas[parabola].value = getPixel<OutputType, flipped>({i, offset});
            parabolas[parabola + 1].begin = std::numeric_limits<OutputType>::infinity();
        }
        for (DimensionType parabola = 0, i = 0; i < length; ++i) {
            while (parabolas[++parabola].begin < i)
                ;
            --parabola;
            auto pixel = getPixel<InputType, flipped>({i, offset}) ? -1 : 1;
            auto value = std::sqrt(parabolas[parabola].value + square(i - parabolas[parabola].apex)) * pixel;
            setPixel<OutputType, flipped>({i, offset}, value);
        }
    }

    void ParabolaEnvelope::transform() {
        assert(input.getWidth() > 0 && input.getHeight() > 0);
        DimensionType length = std::max(input.getWidth(), input.getHeight());
        parabolas.reset(new Parabola[length + 1]);
        lineBuffer.reset(new OutputType[length]);

        for(DimensionType x = 0; x < input.getWidth(); ++x)
            edgeDetection<true, true>(x, input.getHeight());

        for(DimensionType y = 0; y < input.getHeight(); ++y) {
            edgeDetection<false, false>(y, input.getWidth());
            transformLine<false>(y, input.getWidth());
        }
    }
}
