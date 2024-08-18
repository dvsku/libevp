#pragma once

#include <type_traits>

namespace libevp {
    template<typename T>
    concept arithmetic = std::is_arithmetic<T>::value;

    template<typename T>
    concept is_enum = std::is_enum<T>::value;
}
