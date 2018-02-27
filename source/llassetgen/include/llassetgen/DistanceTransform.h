#pragma once

#include <llassetgen/Vec2.h>
#include <llassetgen/llassetgen.h>
#include <llassetgen/llassetgen_api.h>

struct FT_Bitmap_;
struct png_struct_def;

namespace llassetgen {
    class LLASSETGEN_API DistanceTransform {
       public:
        using DimensionType = int;
        using InputType = unsigned char;
        using OutputType = float;
        using PositionType = Vec2<DimensionType>;

       protected:
        std::unique_ptr<InputType[]> input;
        std::unique_ptr<OutputType[]> output;
        template <typename PixelType>
        LLASSETGEN_NO_EXPORT void exportPngInternal(png_struct_def*, OutputType, OutputType);

       public:
        DimensionType width, height;

        DistanceTransform() : width(0), height(0) {}

        DistanceTransform(DimensionType _width, DimensionType _height) { resetInput(_width, _height, true); }

        LLASSETGEN_NO_EXPORT void resetInput(DimensionType width, DimensionType height, bool clear);
        LLASSETGEN_NO_EXPORT bool inputAt(DimensionType offset);
        LLASSETGEN_NO_EXPORT bool inputAt(PositionType pos);
        LLASSETGEN_NO_EXPORT bool inputAtClamped(PositionType pos);
        LLASSETGEN_NO_EXPORT void inputAt(PositionType pos, bool bit);
        LLASSETGEN_NO_EXPORT OutputType& outputAt(DimensionType offset);
        LLASSETGEN_NO_EXPORT OutputType& outputAt(PositionType pos);
        LLASSETGEN_NO_EXPORT OutputType outputAtClamped(PositionType pos);

        void importFreeTypeBitmap(FT_Bitmap_* bitmap, DimensionType padding);
        void importPng(std::string path);
        void exportPng(std::string path, OutputType blackDistance, OutputType whiteDistance, DimensionType bitDepth);

        virtual void transform() = 0;
    };

    class LLASSETGEN_API DeadReckoning : public DistanceTransform {
        std::unique_ptr<PositionType[]> posBuffer;

        LLASSETGEN_NO_EXPORT PositionType& posAt(PositionType pos);
        LLASSETGEN_NO_EXPORT void transformAt(PositionType pos, PositionType target, OutputType distance);

       public:
        void transform();
    };

    class LLASSETGEN_API ParabolaEnvelope : public DistanceTransform {
        struct Parabola {
            DimensionType apex;
            OutputType begin, value;
        };
        std::unique_ptr<Parabola[]> parabolas;
        std::unique_ptr<OutputType[]> lineBuffer;

        template <bool fill>
        LLASSETGEN_NO_EXPORT void edgeDetection(DimensionType offset, DimensionType pitch, DimensionType length);
        LLASSETGEN_NO_EXPORT void transformLine(DimensionType offset, DimensionType pitch, DimensionType length);

       public:
        void transform();
    };
}
