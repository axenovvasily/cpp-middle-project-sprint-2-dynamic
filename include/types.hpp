#pragma once

#include <string>
#include <tuple>
#include <utility>

namespace stdx::details {

// Класс для хранения ошибки неуспешного сканирования

struct scan_error {
    std::string message;
};

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
