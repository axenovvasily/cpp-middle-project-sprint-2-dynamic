#pragma once

#include "parse.hpp"

#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace stdx::details {

// Index — целочисленный параметр шаблона: номер плейсхолдера в форматирующей строке.
template <std::size_t Index, typename T>
constexpr std::expected<T, scan_error> parse_placeholder(std::string_view source, std::string_view spec) {
    auto parsed = parse_value_with_format<T>(source, spec);
    if (!parsed) {
        return std::unexpected(scan_error{"Placeholder " + std::to_string(Index) + ": " + parsed.error().message});
    }
    return std::move(*parsed);
}

template <typename... Ts, std::size_t... Is>
constexpr std::expected<scan_result<Ts...>, scan_error> collect_values(const std::vector<std::string_view> &specs,
                                                                       const std::vector<std::string_view> &sources,
                                                                       std::index_sequence<Is...>) {
    std::tuple<std::expected<std::remove_cv_t<Ts>, scan_error>...> parsed{
        parse_placeholder<Is, std::remove_cv_t<Ts>>(sources[Is], specs[Is])...};

    std::optional<scan_error> error;
    (
        [&]<std::size_t I>() {
            if (error) {
                return;
            }
            if (!std::get<I>(parsed)) {
                error = std::get<I>(parsed).error();
            }
        }.template operator()<Is>(),
        ...);

    if (error) {
        return std::unexpected(*error);
    }
    return scan_result<Ts...>{std::tuple<Ts...>{static_cast<Ts>(std::move(*std::get<Is>(parsed)))...}};
}

}  // namespace stdx::details

namespace stdx {

template <typename... Ts>
constexpr std::expected<details::scan_result<Ts...>, details::scan_error> scan(std::string_view input,
                                                                               std::string_view format) {
    constexpr auto types_are_scannable = []<typename... Us>() { return (details::scannable_type<Us> && ...); };
    if constexpr (!types_are_scannable.template operator()<Ts...>()) {
        return std::unexpected(details::scan_error{"Unsupported type"});
    } else {
        if (auto braces = details::check_format_braces(format); !braces) {
            return std::unexpected(std::move(braces.error()));
        }

        auto parts = details::parse_sources(input, format);
        if (!parts) {
            return std::unexpected(std::move(parts.error()));
        }

        const auto &[specs, sources] = *parts;
        if (specs.size() != sizeof...(Ts)) {
            return std::unexpected(details::scan_error{"Number of placeholders does not match the number of types"});
        }
        if constexpr (sizeof...(Ts) == 0) {
            const bool no_captured_text = sources.empty() || (sources.size() == 1 && sources.front().empty());
            if (!no_captured_text) {
                return std::unexpected(
                    details::scan_error{"Unformatted text in input and format string are different"});
            }
            return details::scan_result<Ts...>{std::tuple<Ts...>{}};
        } else if (sources.size() != specs.size()) {
            return std::unexpected(details::scan_error{"Failed to associate input fragments with placeholders"});
        } else {
            return details::collect_values<Ts...>(specs, sources, std::index_sequence_for<Ts...>{});
        }
    }
}

}  // namespace stdx
