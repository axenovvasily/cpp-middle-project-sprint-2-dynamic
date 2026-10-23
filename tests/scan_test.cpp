#include <gtest/gtest.h>

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "scan.hpp"

namespace {

std::string ErrorText(const auto &result) {
    if (result) {
        return {};
    }
    return result.error().message;
}

struct NotScannable {};

}  // namespace

TEST(ScanTest, ParsesAssignmentExample) {
    const auto result =
        stdx::scan<std::int32_t, double>("I want to sum 42 and 3.14 numbers.", "I want to sum {} and {%f} numbers.");
    ASSERT_TRUE(result.has_value()) << ErrorText(result);
    const auto [sum, pi] = result->values();
    EXPECT_EQ(sum, 42);
    EXPECT_DOUBLE_EQ(pi, 3.14);
}

TEST(ScanTest, ParsesAllSignedIntegers) {
    const auto result = stdx::scan<std::int8_t, std::int16_t, std::int32_t, std::int64_t>(
        "-128 32767 -1 9223372036854775807", "{%d} {%d} {%d} {%d}");
    ASSERT_TRUE(result.has_value()) << ErrorText(result);
    const auto [i8, i16, i32, i64] = result->values();
    EXPECT_EQ(i8, static_cast<std::int8_t>(-128));
    EXPECT_EQ(i16, static_cast<std::int16_t>(32767));
    EXPECT_EQ(i32, -1);
    EXPECT_EQ(i64, static_cast<std::int64_t>(9223372036854775807LL));
}

TEST(ScanTest, ParsesAllUnsignedIntegers) {
    const auto result = stdx::scan<std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t>(
        "0 65535 42 18446744073709551615", "{%u} {%u} {%u} {%u}");
    ASSERT_TRUE(result.has_value()) << ErrorText(result);
    const auto [u8, u16, u32, u64] = result->values();
    EXPECT_EQ(u8, 0);
    EXPECT_EQ(u16, 65535);
    EXPECT_EQ(u32, 42u);
    EXPECT_EQ(u64, std::uint64_t{18446744073709551615ULL});
}

TEST(ScanTest, ParsesFloatingPoint) {
    const auto result = stdx::scan<float, double>("3.5 -2.5e1", "{%f} {}");
    ASSERT_TRUE(result.has_value()) << ErrorText(result);
    const auto [single, doubled] = result->values();
    EXPECT_FLOAT_EQ(single, 3.5f);
    EXPECT_DOUBLE_EQ(doubled, -25.0);
}

TEST(ScanTest, ParsesStringAndStringView) {
    const std::string input = "user ada loves templates";
    const auto result = stdx::scan<std::string, std::string_view>(input, "user {%s} loves {}");
    ASSERT_TRUE(result.has_value()) << ErrorText(result);
    const auto [name, interest] = result->values();
    EXPECT_EQ(name, "ada");
    EXPECT_EQ(interest, "templates");
}

TEST(ScanTest, ParsesCvQualifiedTypes) {
    const auto result =
        stdx::scan<const std::int32_t, volatile double, const std::string>("7 1.5 text", "{%d} {%f} {%s}");
    ASSERT_TRUE(result.has_value()) << ErrorText(result);
    const auto stored = result->values();
    EXPECT_EQ(std::get<0>(stored), 7);
    const double pi_like = std::get<1>(stored);
    EXPECT_DOUBLE_EQ(pi_like, 1.5);
    EXPECT_EQ(std::get<2>(stored), "text");
}

TEST(ScanTest, ParsesEmptySpecifierByTargetType) {
    const auto result = stdx::scan<std::int32_t, std::string, double>("10 word 2.5", "{} {} {}");
    ASSERT_TRUE(result.has_value()) << ErrorText(result);
    EXPECT_EQ(std::get<0>(result->values()), 10);
    EXPECT_EQ(std::get<1>(result->values()), "word");
    EXPECT_DOUBLE_EQ(std::get<2>(result->values()), 2.5);
}

TEST(ScanTest, ParsesRuntimeStrings) {
    const std::string input = "age 27";
    const std::string format = "age {%d}";
    const auto result = stdx::scan<std::int32_t>(input, format);
    ASSERT_TRUE(result.has_value()) << ErrorText(result);
    EXPECT_EQ(std::get<0>(result->values()), 27);
}

TEST(ScanTest, StringViewPointsIntoTheSource) {
    const std::string input = "name Ada";
    const auto result = stdx::scan<std::string_view>(input, "name {%s}");
    ASSERT_TRUE(result.has_value()) << ErrorText(result);
    const std::string_view view = std::get<0>(result->values());
    EXPECT_EQ(view, "Ada");
    EXPECT_EQ(view.data(), input.data() + input.find("Ada"));
}

TEST(ScanTest, StringCopiesTheSource) {
    std::string input = "name Ada";
    const auto result = stdx::scan<std::string>(input, "name {%s}");
    ASSERT_TRUE(result.has_value()) << ErrorText(result);
    input = "rewritten";
    EXPECT_EQ(std::get<0>(result->values()), "Ada");
}

TEST(ScanTest, ParsesEmptyStringField) {
    const auto result = stdx::scan<std::string>("[]", "[{%s}]");
    ASSERT_TRUE(result.has_value()) << ErrorText(result);
    EXPECT_TRUE(std::get<0>(result->values()).empty());
}

TEST(ScanTest, ParsesPlusSignAndLeadingZeros) {
    const auto result = stdx::scan<std::int32_t, std::uint16_t>("+007 00042", "{%d} {%u}");
    ASSERT_TRUE(result.has_value()) << ErrorText(result);
    EXPECT_EQ(std::get<0>(result->values()), 7);
    EXPECT_EQ(std::get<1>(result->values()), 42);
}

TEST(ScanTest, ParsesFormatWithoutPlaceholders) {
    const auto result = stdx::scan<>("ready", "ready");
    ASSERT_TRUE(result.has_value()) << ErrorText(result);
    EXPECT_EQ(std::tuple_size_v<std::remove_cvref_t<decltype(result->values())>>, 0u);
}

TEST(ScanTest, RejectsSpecifierMismatch) {
    const auto signed_with_unsigned = stdx::scan<std::uint32_t>("10", "{%d}");
    ASSERT_FALSE(signed_with_unsigned.has_value());
    EXPECT_EQ(signed_with_unsigned.error().message,
              "Placeholder 0: Conversion specifier does not match the target type");

    const auto float_with_string = stdx::scan<std::string>("3.14", "{%f}");
    ASSERT_FALSE(float_with_string.has_value());
    EXPECT_NE(float_with_string.error().message.find("does not match"), std::string::npos);

    const auto string_with_int = stdx::scan<std::int32_t>("abc", "{%s}");
    ASSERT_FALSE(string_with_int.has_value());
    EXPECT_NE(string_with_int.error().message.find("does not match"), std::string::npos);

    const auto unsigned_with_signed = stdx::scan<std::int32_t>("10", "{%u}");
    ASSERT_FALSE(unsigned_with_signed.has_value());
    EXPECT_NE(unsigned_with_signed.error().message.find("does not match"), std::string::npos);
}

TEST(ScanTest, RejectsInvalidSpecifier) {
    const auto unknown = stdx::scan<std::int32_t>("10", "{%q}");
    ASSERT_FALSE(unknown.has_value());
    EXPECT_EQ(unknown.error().message, "Placeholder 0: Invalid conversion specifier");

    const auto missing_percent = stdx::scan<std::int32_t>("10", "{d}");
    ASSERT_FALSE(missing_percent.has_value());
    EXPECT_EQ(missing_percent.error().message, "Placeholder 0: Invalid conversion specifier");
}

TEST(ScanTest, RejectsUnsupportedTypes) {
    EXPECT_FALSE(stdx::scan<bool>("1", "{}").has_value());
    EXPECT_FALSE(stdx::scan<char>("A", "{%s}").has_value());
    EXPECT_FALSE(stdx::scan<long double>("1.0", "{%f}").has_value());
    EXPECT_FALSE(stdx::scan<NotScannable>("x", "{}").has_value());
    EXPECT_FALSE(stdx::scan<std::int32_t *>("1", "{}").has_value());
    EXPECT_EQ(stdx::scan<bool>("1", "{}").error().message, "Unsupported type");
}

TEST(ScanTest, RejectsReferenceTypes) {
    const auto lvalue = stdx::scan<std::int32_t &>("7", "{%d}");
    ASSERT_FALSE(lvalue.has_value());
    EXPECT_EQ(lvalue.error().message, "Unsupported type");

    const auto const_lvalue = stdx::scan<const std::int32_t &>("7", "{%d}");
    ASSERT_FALSE(const_lvalue.has_value());
    EXPECT_EQ(const_lvalue.error().message, "Unsupported type");
}

TEST(ScanTest, RejectsOutOfRangeValues) {
    const auto too_wide = stdx::scan<std::int8_t>("128", "{%d}");
    ASSERT_FALSE(too_wide.has_value());
    EXPECT_NE(too_wide.error().message.find("out of range"), std::string::npos);

    const auto too_narrow = stdx::scan<std::int8_t>("-129", "{}");
    ASSERT_FALSE(too_narrow.has_value());
    EXPECT_NE(too_narrow.error().message.find("out of range"), std::string::npos);

    const auto unsigned_overflow = stdx::scan<std::uint8_t>("256", "{%u}");
    ASSERT_FALSE(unsigned_overflow.has_value());
    EXPECT_NE(unsigned_overflow.error().message.find("out of range"), std::string::npos);
}

TEST(ScanTest, RejectsInvalidNumbers) {
    EXPECT_FALSE(stdx::scan<std::int32_t>("", "{%d}").has_value());
    EXPECT_FALSE(stdx::scan<std::int32_t>("42abc", "{}").has_value());
    EXPECT_FALSE(stdx::scan<std::int32_t>(" 42", "{%d}").has_value());
    EXPECT_FALSE(stdx::scan<std::uint32_t>("-1", "{%u}").has_value());
    EXPECT_FALSE(stdx::scan<double>("3.14.15", "{%f}").has_value());
    EXPECT_EQ(stdx::scan<std::int32_t>("", "{%d}").error().message, "Placeholder 0: Empty numeric value");
}

TEST(ScanTest, RejectsPlaceholderCountMismatch) {
    EXPECT_FALSE(stdx::scan<std::int32_t>("1 2", "{} {}").has_value());
    EXPECT_FALSE((stdx::scan<std::int32_t, std::int32_t>("1", "{}").has_value()));
    EXPECT_FALSE(stdx::scan<>("value", "{}").has_value());
    EXPECT_EQ(stdx::scan<std::int32_t>("1 2", "{} {}").error().message,
              "Number of placeholders does not match the number of types");
}

TEST(ScanTest, RejectsLiteralMismatch) {
    const auto different = stdx::scan<std::int32_t>("value 5", "id {%d}");
    ASSERT_FALSE(different.has_value());
    EXPECT_EQ(different.error().message, "Unformatted text in input and format string are different");

    const auto trailing = stdx::scan<std::int32_t>("n 1! extra", "n {%d}!");
    ASSERT_FALSE(trailing.has_value());
    EXPECT_EQ(trailing.error().message, "Unformatted text in input and format string are different");

    const auto empty_format = stdx::scan<>("data", "");
    ASSERT_FALSE(empty_format.has_value());
    EXPECT_EQ(empty_format.error().message, "Unformatted text in input and format string are different");
}

TEST(ScanTest, RejectsMalformedFormat) {
    EXPECT_EQ(stdx::scan<std::int32_t>("1", "{%d").error().message, "Unclosed '{' in format string");
    EXPECT_EQ(stdx::scan<std::int32_t>("1", "1}").error().message, "Unmatched '}' in format string");
    EXPECT_EQ(stdx::scan<std::int32_t>("1", "{{%d}}").error().message, "Nested '{' in format string");
}

TEST(ScanTest, ReportsTheFirstPlaceholderError) {
    const auto result = stdx::scan<std::int32_t, std::int32_t>("xx 1", "{} {}");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().message, "Placeholder 0: Invalid numeric value");

    const auto second = stdx::scan<std::int32_t, std::int32_t>("1 xx", "{} {}");
    ASSERT_FALSE(second.has_value());
    EXPECT_EQ(second.error().message, "Placeholder 1: Invalid numeric value");
}

TEST(ScanTest, RejectsAdjacentPlaceholders) {
    const auto result = stdx::scan<std::int32_t, std::int32_t>("12", "{}{}");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().message, "Failed to associate input fragments with placeholders");
}

constexpr auto kCompileTimeScan = stdx::scan<std::int32_t>("answer 42", "answer {%d}");
static_assert(kCompileTimeScan.has_value());
static_assert(std::get<0>(kCompileTimeScan->values()) == 42);
