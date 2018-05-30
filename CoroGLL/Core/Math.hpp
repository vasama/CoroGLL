#pragma once

#include "Types.hpp"

#include <type_traits>

namespace CoroGLL {
	
template<typename T>
constexpr std::enable_if_t<std::is_unsigned_v<T>, T> HighBit = ~(~(T)0 >> 1);

template<typename T>
constexpr std::enable_if_t<std::is_unsigned_v<T>, T> IsPowerOfTwoOrZero(T x)
{
	return (x & (x - 1)) == 0;
}

template<typename T>
std::enable_if_t<std::is_unsigned_v<T>, T> NextPowerOfTwo(T x)
{
	if (IsPowerOfTwoOrZero(x))
		return x;

	//OPTIMIZE: use clz
	for (T i = HighBit<T>;;)
	{
		T n = i >>= 1;
		if (x & n) return i;
		i = n;
	}
}

} // namespace CoroGLL
