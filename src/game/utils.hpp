//
// Created by andy on 6/4/2025.
//

#pragma once

#include <cstddef>
#include <array>
#include <functional>

namespace game {
    template<typename T, std::size_t...Is>
    std::array<T, sizeof...(Is)> array_from_fn(const std::function<T(std::size_t)> &f, std::index_sequence<Is...>) {
        return { ((void)Is, f(Is))... };
    }


    template<typename T, std::size_t N>
    std::array<T, N> array_from_fn(const std::function<T(std::size_t)> &f) {
        return array_from_fn<T>(f, std::make_index_sequence<N>());
    }
}