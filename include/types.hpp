#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace stdx::details {

// Класс для хранения ошибки неуспешного сканирования

struct scan_error {
    std::string message;
};

// Трейты поддерживаемых scan типов. Спецификатор совпадает с scanf: d, u, f, s.
// Частичные специализации распространяют те же свойства на cv-квалифицированные версии.

template <typename T>
struct scan_traits {
    static constexpr bool supported = false;
    static constexpr char specifier = '\0';
};

template <>
struct scan_traits<std::int8_t> {
    static constexpr bool supported = true;
    static constexpr char specifier = 'd';
};

template <>
struct scan_traits<std::int16_t> {
    static constexpr bool supported = true;
    static constexpr char specifier = 'd';
};

template <>
struct scan_traits<std::int32_t> {
    static constexpr bool supported = true;
    static constexpr char specifier = 'd';
};

template <>
struct scan_traits<std::int64_t> {
    static constexpr bool supported = true;
    static constexpr char specifier = 'd';
};

template <>
struct scan_traits<std::uint8_t> {
    static constexpr bool supported = true;
    static constexpr char specifier = 'u';
};

template <>
struct scan_traits<std::uint16_t> {
    static constexpr bool supported = true;
    static constexpr char specifier = 'u';
};

template <>
struct scan_traits<std::uint32_t> {
    static constexpr bool supported = true;
    static constexpr char specifier = 'u';
};

template <>
struct scan_traits<std::uint64_t> {
    static constexpr bool supported = true;
    static constexpr char specifier = 'u';
};

template <>
struct scan_traits<float> {
    static constexpr bool supported = true;
    static constexpr char specifier = 'f';
};

template <>
struct scan_traits<double> {
    static constexpr bool supported = true;
    static constexpr char specifier = 'f';
};

template <>
struct scan_traits<std::string> {
    static constexpr bool supported = true;
    static constexpr char specifier = 's';
};

template <>
struct scan_traits<std::string_view> {
    static constexpr bool supported = true;
    static constexpr char specifier = 's';
};

template <typename T>
struct scan_traits<const T> : scan_traits<T> {};

template <typename T>
struct scan_traits<volatile T> : scan_traits<T> {};

template <typename T>
struct scan_traits<const volatile T> : scan_traits<T> {};

template <typename T>
inline constexpr bool is_scannable_v = scan_traits<T>::supported;

template <typename T>
concept scannable_type = is_scannable_v<T>;

template <typename T>
concept signed_scan_type = scannable_type<T> && scan_traits<T>::specifier == 'd';

template <typename T>
concept unsigned_scan_type = scannable_type<T> && scan_traits<T>::specifier == 'u';

template <typename T>
concept floating_scan_type = scannable_type<T> && scan_traits<T>::specifier == 'f';

template <typename T>
concept string_scan_type = scannable_type<T> && scan_traits<T>::specifier == 's';

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
