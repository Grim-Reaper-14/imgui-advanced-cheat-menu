#pragma once

#include <cstddef>

namespace RevivalHeaderData {
inline constexpr const char* Parts[] = {
#include "RevivalHeaderPart0.inc"
#include "RevivalHeaderPart1.inc"
#include "RevivalHeaderPart2.inc"
#include "RevivalHeaderPart3.inc"
};

inline constexpr std::size_t PartCount = sizeof(Parts) / sizeof(Parts[0]);
}
