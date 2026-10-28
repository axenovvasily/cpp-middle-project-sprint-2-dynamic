#pragma once

#include <concepts>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace stdx::details {

// Класс для хранения ошибки неуспешного сканирования

struct scan_error {
    std::string message;
};

template <typename T, typename... Allowed>
concept scan_type_in = (std::same_as<std::remove_cv_t<T>, Allowed> || ...);

// Поддерживаемые scan-типы. Спецификатор совпадает с scanf: d, u, f, s.

template <typename T>
concept signed_scan_type = scan_type_in<T, std::int8_t, std::int16_t, std::int32_t, std::int64_t>;

template <typename T>
concept unsigned_scan_type = scan_type_in<T, std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t>;

template <typename T>
concept floating_scan_type = scan_type_in<T, float, double>;

template <typename T>
concept string_scan_type = scan_type_in<T, std::string, std::string_view>;

template <typename T>
concept scannable_type = signed_scan_type<T> || unsigned_scan_type<T> || floating_scan_type<T> || string_scan_type<T>;

// Шаблонный класс для хранения результатов успешного сканирования

template <typename... Ts>
class scan_result {
public:
    using values_type = std::tuple<Ts...>;

    explicit constexpr scan_result(values_type values) : values_(std::move(values)) {}

    [[nodiscard]] constexpr values_type &values() & noexcept { return values_; }
    [[nodiscard]] constexpr const values_type &values() const & noexcept { return values_; }

private:
    values_type values_;
};

}  // namespace stdx::details
