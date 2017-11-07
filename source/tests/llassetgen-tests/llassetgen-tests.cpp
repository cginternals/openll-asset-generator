#include <gmock/gmock.h>
#include <llassetgen/llassetgen.h>

class llassetgen_tests : public testing::Test {

};

TEST_F(llassetgen_tests, CheckSomeResults) {
    EXPECT_EQ(1, 1);
}
