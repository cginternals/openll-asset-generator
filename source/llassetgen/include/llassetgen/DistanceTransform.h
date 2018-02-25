#pragma once

#include <llassetgen/Image.h>

namespace llassetgen {
    class DistanceTransform {
       public:
        using DimensionType = size_t;
        using InputType = uint8_t;
        using OutputType = float;
        using PositionType = Vec2<DimensionType>;

       protected:
        template<typename PixelType, bool flipped = false, bool invalidBounds = false>
        PixelType getPixel(PositionType pos);
        template<typename PixelType, bool flipped = false>
        void setPixel(PositionType pos, PixelType value);

       public:
        const Image& input;
        const Image& output;
        DistanceTransform(const Image& _input, const Image& _output);

        virtual void transform() = 0;
    };

    class DeadReckoning : public DistanceTransform {
        std::unique_ptr<PositionType[]> posBuffer;

        PositionType& posAt(PositionType pos);
        void transformAt(PositionType pos, PositionType target, OutputType distance);

       public:
        LLASSETGEN_API DeadReckoning(const Image& _input, const Image& _output) :DistanceTransform(_input, _output) {}
        LLASSETGEN_API void transform();
    };

    class ParabolaEnvelope : public DistanceTransform {
        struct Parabola {
            DimensionType apex;
            OutputType begin;
            OutputType value;
        };
        std::unique_ptr<Parabola[]> parabolas;
        std::unique_ptr<OutputType[]> lineBuffer;

        template<bool flipped, bool fill>
        void edgeDetection(DimensionType offset, DimensionType length);
        template<bool flipped>
        void transformLine(DimensionType offset, DimensionType length);

       public:
        LLASSETGEN_API ParabolaEnvelope(const Image& _input, const Image& _output) :DistanceTransform(_input, _output) {}
        LLASSETGEN_API void transform();
    };
}
