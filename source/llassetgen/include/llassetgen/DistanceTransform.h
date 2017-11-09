struct FT_Bitmap_;
struct png_struct_def;

namespace llassetgen {
    class DistanceTransform {
        public:
        using DimensionType = unsigned int;
        using InputType = unsigned char;
        using OutputType = float;

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
        InputType inputAt(DimensionType x, DimensionType y);
        void inputAt(DimensionType x, DimensionType y, InputType bit);
        OutputType& outputAt(DimensionType x, DimensionType y);

        void importFreeTypeBitmap(FT_Bitmap_* bitmap, DimensionType padding);
        void importPng(std::string path);
        void exportPng(std::string path, OutputType blackDistance, OutputType whiteDistance, DimensionType bitDepth);

        void transform() {};
    };
}
