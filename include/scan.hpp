#pragma once

#include "parse.hpp"

#include <cstddef>
#include <expected>
#include <optional>
#include <string_view>
#include <tuple>
#include <utility>

namespace stdx::details {

template <typename... Ts, std::size_t... Is>
constexpr std::expected<scan_result<Ts...>, scan_error> collect_values(const std::vector<std::string_view> &specs,
                                                                       const std::vector<std::string_view> &sources,
                                                                       std::index_sequence<Is...>) {
    std::tuple<std::expected<std::remove_cv_t<Ts>, scan_error>...> parsed{
        parse_value_with_format<std::remove_cv_t<Ts>>(sources[Is], specs[Is])...};

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
    auto parts = details::parse_sources(input, format);
    if (!parts) {
        return std::unexpected(std::move(parts.error()));
    }

    const auto &[specs, sources] = *parts;
    if (specs.size() != sizeof...(Ts) || sources.size() != sizeof...(Ts)) {
        return std::unexpected(details::scan_error{"Number of placeholders does not match the number of types"});
    }
    return details::collect_values<Ts...>(specs, sources, std::index_sequence_for<Ts...>{});
}

}  // namespace stdx
