struct FT_Bitmap_;
struct png_struct_def;

namespace llassetgen {
    class DistanceTransform {
        public:
        using DimensionType = unsigned int;
        using InputType = unsigned char;
        using OutputType = float;
        struct PositionType {
            DimensionType x, y;
            template<typename T>
            PositionType(T _x, T _y) :x(_x), y(_y) {}
            PositionType() {}
            PositionType& operator+=(const PositionType& other) {
                x += other.x;
                y += other.y;
                return *this;
            }
            PositionType operator-() const {
                return {-x, -y};
            }
        };

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
        InputType inputAt(PositionType pos);
        InputType inputAtClamped(PositionType pos);
        void inputAt(PositionType pos, InputType bit);
        OutputType& outputAt(PositionType pos);
        OutputType outputAtClamped(PositionType pos);

        void importFreeTypeBitmap(FT_Bitmap_* bitmap, DimensionType padding);
        void importPng(std::string path);
        void exportPng(std::string path, OutputType blackDistance, OutputType whiteDistance, DimensionType bitDepth);

        virtual void transform() = 0;
    };

    class DeadReackoning : public DistanceTransform {
        std::unique_ptr<PositionType[]> posBuffer;
        PositionType& posAt(PositionType pos);
        void transformAt(PositionType pos, PositionType target, OutputType distance);

        public:
        void transform();
    };
}
