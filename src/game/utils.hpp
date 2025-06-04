//
// Created by andy on 6/4/2025.
//

#pragma once

#include <cstddef>
#include <array>
#include <functional>

namespace game {
    template<typename T, std::size_t N>
    std::array<T, N> array_from_fn(const std::function<T(std::size_t)> &f) {
        T *arr = new T[N];
        for (std::size_t i = 0 ; i < N ; ++i) {
            arr[i] = f(i);
        }
///https://stackoverflow.com/questions/46899731/initializing-an-stdarray-of-non-default-constructible-elements
        return std::make_index_sequence<>);
    }
}