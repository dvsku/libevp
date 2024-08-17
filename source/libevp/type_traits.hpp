#pragma once

#include <type_traits>

namespace libevp {
    template<typename T>
    concept arithmetic = std::is_arithmetic<T>::value;
}
