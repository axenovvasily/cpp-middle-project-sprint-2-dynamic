#pragma once

#include <charconv>
#include <concepts>
#include <expected>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include "types.hpp"

namespace stdx::details {

// Функция для парсинга значения с учетом спецификатора формата
template <typename T>
constexpr std::expected<T, scan_error> parse_value_with_format(std::string_view input, std::string_view fmt) {
    if constexpr (!scannable_type<T> || !std::same_as<T, std::remove_cv_t<T>>) {
        return std::unexpected(scan_error{"Unsupported type"});
    } else {
        if (!fmt.empty()) {
            if (fmt.size() != 2 || fmt.front() != '%') {
                return std::unexpected(scan_error{"Invalid conversion specifier"});
            }
            const char spec = fmt.back();
            const bool known = spec == 'd' || spec == 'u' || spec == 'f' || spec == 's';
            const bool matches = (spec == 'd' && signed_scan_type<T>) || (spec == 'u' && unsigned_scan_type<T>) ||
                                 (spec == 'f' && floating_scan_type<T>) || (spec == 's' && string_scan_type<T>);
            if (!known) {
                return std::unexpected(scan_error{"Invalid conversion specifier"});
            }
            if (!matches) {
                return std::unexpected(scan_error{"Conversion specifier does not match the target type"});
            }
        }

        if constexpr (signed_scan_type<T> || unsigned_scan_type<T>) {
            if (input.empty()) {
                return std::unexpected(scan_error{"Empty numeric value"});
            }
            const char *first = input.data();
            const char *const last = input.data() + input.size();
            if (*first == '+') {
                ++first;
                if (first == last) {
                    return std::unexpected(scan_error{"Invalid numeric value"});
                }
            }
            T value{};
            const auto parsed = std::from_chars(first, last, value);
            if (parsed.ec == std::errc::result_out_of_range) {
                return std::unexpected(scan_error{"Numeric value is out of range"});
            }
            if (parsed.ec != std::errc{} || parsed.ptr != last) {
                return std::unexpected(scan_error{"Invalid numeric value"});
            }
            return value;
        } else if constexpr (floating_scan_type<T>) {
            if (input.empty()) {
                return std::unexpected(scan_error{"Empty numeric value"});
            }
            T value{};
            const char *const first = input.data();
            const char *const last = input.data() + input.size();
            const auto parsed = std::from_chars(first, last, value, std::chars_format::general);
            if (parsed.ec == std::errc::result_out_of_range) {
                return std::unexpected(scan_error{"Numeric value is out of range"});
            }
            if (parsed.ec != std::errc{} || parsed.ptr != last) {
                return std::unexpected(scan_error{"Invalid numeric value"});
            }
            return value;
        } else {
            return T{input};
        }
    }
}

// Функция для проверки корректности входных данных и выделения из обеих строк интересующих данных для парсинга
template <typename... Ts>
std::expected<std::pair<std::vector<std::string_view>, std::vector<std::string_view>>, scan_error>
parse_sources(std::string_view input, std::string_view format) {
    std::vector<std::string_view> format_parts;  // Части формата между {}
    std::vector<std::string_view> input_parts;
    size_t start = 0;
    while (true) {
        size_t open = format.find('{', start);
        if (open == std::string_view::npos) {
            break;
        }
        size_t close = format.find('}', open);
        if (close == std::string_view::npos) {
            break;
        }

        // Если между предыдущей } и текущей { есть текст,
        // проверяем его наличие во входной строке
        if (open > start) {
            std::string_view between = format.substr(start, open - start);
            auto pos = input.find(between);
            if (input.size() < between.size() || pos == std::string_view::npos) {
                return std::unexpected(scan_error{"Unformatted text in input and format string are different"});
            }
            if (start != 0) {
                input_parts.emplace_back(input.substr(0, pos));
            }

            input = input.substr(pos + between.size());
        }

        // Сохраняем спецификатор формата (то, что между {})
        format_parts.push_back(format.substr(open + 1, close - open - 1));
        start = close + 1;
    }

    // Проверяем оставшийся текст после последней }
    if (start < format.size()) {
        std::string_view remaining_format = format.substr(start);
        auto pos = input.find(remaining_format);
        if (input.size() < remaining_format.size() || pos == std::string_view::npos) {
            return std::unexpected(scan_error{"Unformatted text in input and format string are different"});
        }
        input_parts.emplace_back(input.substr(0, pos));
        input = input.substr(pos + remaining_format.size());
    } else {
        input_parts.emplace_back(input);
    }
    return std::pair{format_parts, input_parts};
}

}  // namespace stdx::details