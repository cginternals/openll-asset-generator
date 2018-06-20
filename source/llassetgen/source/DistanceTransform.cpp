#include <cassert>
#include <cmath>

#include <llassetgen/DistanceTransform.h>

namespace llassetgen {
    template <typename PixelType, bool flipped, bool invalidBounds>
    PixelType DistanceTransform::getPixel(PositionType pos) {
        const Image& image = std::is_same<PixelType, InputType>::value ? input : output;
        if (invalidBounds && !image.isValid(pos)) {
            return 0;
        }
        if (flipped) {
            std::swap(pos.x, pos.y);
        }
        return image.getPixel<PixelType>(pos);
    }

    template <typename PixelType, bool flipped>
    void DistanceTransform::setPixel(PositionType pos, PixelType value) {
        const Image& image = std::is_same<PixelType, InputType>::value ? input : output;
        if (flipped) {
            std::swap(pos.x, pos.y);
        }
        image.setPixel<PixelType>(pos, value);
    }

    DistanceTransform::DistanceTransform(const Image& _input, const Image& _output) : input(_input), output(_output) {
        assert(input.getWidth() == output.getWidth() && input.getHeight() == output.getHeight() &&
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

        for (DimensionType y = 0; y < input.getHeight(); ++y) {
            for (DimensionType x = 0; x < input.getWidth(); ++x) {
                PositionType pos = {x, y};
                bool center = getPixel<InputType, false>(pos);
                posAt(pos) = pos;
                setPixel<OutputType>(pos, (center && (getPixel<InputType, false, true>({x - 1, y}) != center ||
                                                      getPixel<InputType, false, true>({x + 1, y}) != center ||
                                                      getPixel<InputType, false, true>({x, y - 1}) != center ||
                                                      getPixel<InputType, false, true>({x, y + 1}) != center))
                                              ? 0
                                              : backgroundVal);
            }
        }

        const OutputType distance[] = {std::sqrt(2.0F), 1.0F, std::sqrt(2.0F), 1.0F};
        const PositionType target[] = {{static_cast<DimensionType>(-1), static_cast<DimensionType>(-1)},
                                       {static_cast<DimensionType>( 0), static_cast<DimensionType>(-1)},
                                       {static_cast<DimensionType>(+1), static_cast<DimensionType>(-1)},
                                       {static_cast<DimensionType>(-1), static_cast<DimensionType>( 0)}};

        for (DimensionType y = 0; y < input.getHeight(); ++y) {
            for (DimensionType x = 0; x < input.getWidth(); ++x) {
                for (DimensionType i = 0; i < 4; ++i) {
                    transformAt({x, y}, target[i], distance[i]);
                }
            }
        }

        for (DimensionType y = 0; y < input.getHeight(); ++y) {
            for (DimensionType x = 0; x < input.getWidth(); ++x) {
                for (DimensionType i = 0; i < 4; ++i) {
                    transformAt({input.getWidth() - x - 1, input.getHeight() - y - 1}, -(target[3 - i]),
                                distance[3 - i]);
                }
            }
        }

        for (DimensionType y = 0; y < input.getHeight(); ++y) {
            for (DimensionType x = 0; x < input.getWidth(); ++x) {
                Vec2<DimensionType> pos(x, y);
                if (getPixel<InputType, false>(pos)) {
                    setPixel<OutputType>(pos, -getPixel<OutputType>(pos));
                }
            }
        }
    }

    template <bool flipped, bool fill>
    void ParabolaEnvelope::edgeDetection(DimensionType offset, DimensionType length) {
        InputType prevInput = 0;
        DimensionType fillBegin = 0;
        for (DimensionType j = 0; j < length; ++j) {
            InputType nextInput = getPixel<InputType, flipped>({j, offset});
            if (nextInput == prevInput) {
                continue;
            }
            prevInput = nextInput;
            DimensionType fillEnd = (nextInput) ? j : j - 1;
            setPixel<OutputType, flipped>({fillEnd, offset}, 0);  // Mark edge
            if (fill) {
                for (DimensionType i = fillBegin; i < fillEnd; ++i)
                    setPixel<OutputType, flipped>(
                        {i, offset},
                        square(fillBegin == 0
                            ? (fillEnd - i)  // Falling slope
                            : ((i < (fillEnd + fillBegin) / 2) ? i - fillBegin + 1 : fillEnd - i)  // Rising and falling slope
                        )
                    );
                fillBegin = fillEnd + 1;
            }
        }
        if (fill) {
            for (DimensionType i = fillBegin; i < length; ++i) {
                setPixel<OutputType, flipped>(
                    {i, offset},
                    (fillBegin == 0)
                    ? std::numeric_limits<OutputType>::max()  // Empty
                    : square(prevInput
                        ? ((i < (length - 1 + fillBegin) / 2)
                            ? i - fillBegin + 1
                            : length - 1 - i)  // Rising and falling slope
                        : (i - fillBegin + 1)  // Rising slope
                    )
                );
            }
        }
        if (prevInput) {
            setPixel<OutputType, flipped>({length - 1, offset}, 0);  // Mark edge
        }
    }

    template <bool flipped>
    void ParabolaEnvelope::transformLine(DimensionType offset, DimensionType length) {
        parabolas[0].apex = 0;
        parabolas[0].begin = -backgroundVal;
        parabolas[0].value = getPixel<OutputType, flipped>({0, offset});
        parabolas[1].begin = +backgroundVal;
        for (DimensionType parabolaIndex = 0, j = 1; j < length; ++j) {
            OutputType parabolaBegin;
            do {
                DimensionType apex = parabolas[parabolaIndex].apex;
                OutputType value = getPixel<OutputType, flipped>({j, offset});
                parabolaBegin =
                    (value + square(j) - (parabolas[parabolaIndex].value + square(apex))) / (2 * (j - apex));
            } while (parabolaBegin <= parabolas[parabolaIndex--].begin);
            parabolaIndex += 2;
            parabolas[parabolaIndex].apex = j;
            parabolas[parabolaIndex].begin = parabolaBegin;
            parabolas[parabolaIndex].value = getPixel<OutputType, flipped>({j, offset});
            parabolas[parabolaIndex + 1].begin = std::numeric_limits<OutputType>::infinity();
        }
        for (DimensionType parabolaIndex = 0, j = 0; j < length; ++j) {
            while (parabolas[++parabolaIndex].begin < j)
                ;
            --parabolaIndex;
            InputType signMask = getPixel<InputType, flipped>({j, offset});
            setPixel<OutputType, flipped>(
                {j, offset},
                std::sqrt(parabolas[parabolaIndex].value + square(j - parabolas[parabolaIndex].apex))
                    * (signMask ? -1 : 1)
            );
        }
    }

    void ParabolaEnvelope::transform() {
        assert(input.getWidth() > 0 && input.getHeight() > 0);
        DimensionType length = std::max(input.getWidth(), input.getHeight());
        parabolas.reset(new Parabola[length + 1]);
        lineBuffer.reset(new OutputType[length]);

        for (DimensionType x = 0; x < input.getWidth(); ++x) {
            edgeDetection<true, true>(x, input.getHeight());
        }

        for (DimensionType y = 0; y < input.getHeight(); ++y) {
            edgeDetection<false, false>(y, input.getWidth());
            transformLine<false>(y, input.getWidth());
        }
    }
}
