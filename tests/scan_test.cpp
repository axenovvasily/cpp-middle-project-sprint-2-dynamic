#include <gtest/gtest.h>

#include <string>
#include <tuple>

#include "scan.hpp"

TEST(ScanTest, SimpleTest) {
    auto result = stdx::scan<std::string>("number", "{}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result->values()), "number");
}
