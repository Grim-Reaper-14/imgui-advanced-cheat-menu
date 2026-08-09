#pragma once

#include <cstddef>
#include <utility>

namespace ResourceLoader {
    std::pair<const void*, std::size_t> getBinary(int resourceId);
}
