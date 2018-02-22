#include <gmock/gmock.h>
#include <llassetgen/llassetgen.h>

using namespace llassetgen;

class llassetgen_tests : public testing::Test {

};

/*TEST_F(llassetgen_tests, DeadReckoning) {
    Image input("input.png"), output(input.get_width(), input.get_height(), sizeof(DistanceTransform::OutputType)*8);
    std::unique_ptr<DistanceTransform> dt(new DeadReckoning(input, output));
    dt->transform();
    output.exportPng<DistanceTransform::OutputType>("DeadReckoning.png", -20, 50);
    EXPECT_EQ(1, 1);
}

TEST_F(llassetgen_tests, ParabolaEnvelope) {
    Image input("input.png"), output(input.get_width(), input.get_height(), sizeof(DistanceTransform::OutputType)*8);
    std::unique_ptr<DistanceTransform> dt(new ParabolaEnvelope(input, output));
    dt->transform();
    output.exportPng<DistanceTransform::OutputType>("ParabolaEnvelope.png", -20, 50);
    EXPECT_EQ(1, 1);
}*/
