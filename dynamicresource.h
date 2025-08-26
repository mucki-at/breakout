//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"

template<typename T, size_t HistorySize>
class DynamicResource
{
public:
    using value_type = T;

private:
    template <typename TU, std::size_t... I>
    constexpr DynamicResource(TU tu, std::index_sequence<I...>) :
        values{(void(I),make_from_tuple<T>(tu))...},
        index(0)
    {
    }

public:
    template <typename... U, typename Indx = std::make_index_sequence<HistorySize>>
    constexpr DynamicResource(U&&... u) : DynamicResource(make_tuple(std::forward<U>(u)...),Indx{})
    {
    }

    inline auto begin() { return values.begin(); }
    inline auto begin() const { return values.begin(); }
    inline auto end() { return values.end(); }
    inline auto end() const { return values.end(); }
    
    inline constexpr const T& current() const noexcept { return values[index]; }
    inline constexpr T& current() noexcept { return values[index]; }
    inline void cycle() const noexcept { index=(index+1)%HistorySize; }

private:
    array<T, HistorySize> values;
    mutable size_t index;
};