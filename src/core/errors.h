#pragma once

#include <stdexcept>
#include <string>

namespace wg {

class AppError : public std::runtime_error {
public:
    explicit AppError(const std::string& message) : std::runtime_error(message) {}
};

} // namespace wg
