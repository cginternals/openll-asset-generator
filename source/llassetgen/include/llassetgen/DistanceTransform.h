#pragma once

#include <llassetgen/Image.h>
#include <llassetgen/Geometry.h>
#include <llassetgen/llassetgen_api.h>

namespace llassetgen {
    class LLASSETGEN_API DistanceTransform {
       public:
        using DimensionType = size_t;
        using InputType = uint8_t;
        using OutputType = float;
        using PositionType = Vec2<DimensionType>;

       protected:
        template<typename PixelType, bool flipped = false, bool invalidBounds = false>
        LLASSETGEN_NO_EXPORT PixelType getPixel(PositionType pos);
        template<typename PixelType, bool flipped = false>
        LLASSETGEN_NO_EXPORT void setPixel(PositionType pos, PixelType value);

       public:
        const Image& input;
        const Image& output;
        DistanceTransform(const Image& _input, const Image& _output);
        LLASSETGEN_NO_EXPORT virtual void transform() = 0;
    };

    class LLASSETGEN_API DeadReckoning : public DistanceTransform {
        std::unique_ptr<PositionType[]> posBuffer;

        LLASSETGEN_NO_EXPORT PositionType& posAt(PositionType pos);
        LLASSETGEN_NO_EXPORT void transformAt(PositionType pos, PositionType target, OutputType distance);

       public:
        DeadReckoning(const Image& _input, const Image& _output) :DistanceTransform(_input, _output) {}
        void transform();
    };

    class LLASSETGEN_API ParabolaEnvelope : public DistanceTransform {
        struct Parabola {
            DimensionType apex;
            OutputType begin;
            OutputType value;
        };
        std::unique_ptr<Parabola[]> parabolas;
        std::unique_ptr<OutputType[]> lineBuffer;

        template<bool flipped, bool fill>
        LLASSETGEN_NO_EXPORT void edgeDetection(DimensionType offset, DimensionType length);
        template<bool flipped>
        LLASSETGEN_NO_EXPORT void transformLine(DimensionType offset, DimensionType length);

       public:
        ParabolaEnvelope(const Image& _input, const Image& _output) :DistanceTransform(_input, _output) {}
        void transform();
    };
}
