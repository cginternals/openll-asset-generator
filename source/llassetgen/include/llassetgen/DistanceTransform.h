#pragma once

#include <llassetgen/llassetgen_api.h>
#include <llassetgen/llassetgen.h>
#include <llassetgen/Vec2.h>

struct FT_Bitmap_;
struct png_struct_def;

namespace llassetgen {
    class DistanceTransform {
        public:
        using DimensionType = int;
        using InputType = unsigned char;
        using OutputType = float;
        using PositionType = Vec2<DimensionType>;

        protected:
        std::unique_ptr<InputType[]> input;
        std::unique_ptr<OutputType[]> output;
        template<typename PixelType>
        void exportPngInternal(png_struct_def*, OutputType, OutputType);

        public:
        DimensionType width, height;

        DistanceTransform()
            :width(0), height(0) {}

        DistanceTransform(DimensionType _width, DimensionType _height) {
            resetInput(_width, _height, true);
        }

        void resetInput(DimensionType width, DimensionType height, bool clear);
        bool inputAt(DimensionType offset);
        bool inputAt(PositionType pos);
        bool inputAtClamped(PositionType pos);
        void inputAt(PositionType pos, bool bit);
        OutputType& outputAt(DimensionType offset);
        OutputType& outputAt(PositionType pos);
        OutputType outputAtClamped(PositionType pos);

        LLASSETGEN_API void importFreeTypeBitmap(FT_Bitmap_* bitmap, DimensionType padding);
        LLASSETGEN_API void importPng(std::string path);
        LLASSETGEN_API void exportPng(std::string path, OutputType blackDistance, OutputType whiteDistance, DimensionType bitDepth);

        virtual void transform() = 0;
    };

    class DeadReckoning : public DistanceTransform {
        std::unique_ptr<PositionType[]> posBuffer;

        PositionType& posAt(PositionType pos);
        void transformAt(PositionType pos, PositionType target, OutputType distance);

        public:
        LLASSETGEN_API void transform();
    };

    class ParabolaEnvelope : public DistanceTransform {
        struct Parabola {
            DimensionType apex;
            OutputType begin, value;
        };
        std::unique_ptr<Parabola[]> parabolas;
        std::unique_ptr<OutputType[]> lineBuffer;

        template<bool fill>
        void edgeDetection(DimensionType offset, DimensionType pitch, DimensionType length);
        void transformLine(DimensionType offset, DimensionType pitch, DimensionType length);

        public:
        void transform();
    };
}
