#pragma once

#include "Debug.hpp"
#include "Types.hpp"

#include <type_traits>

namespace CoroGLL {

template<typename T>
struct Span
{
	Span()
		: m_size(0)
	{
	}

	Span(T* data, iword size)
		: m_data(data), m_size(size)
	{
	}

	template<typename TIn, iword TSize,
		typename = std::enable_if_t<std::is_convertible_v<TIn*, T*>>>
	Span(TIn(&data)[TSize])
		: m_data(data), m_size(TSize)
	{
	}

	template<typename TIn,
		typename = std::enable_if_t<std::is_convertible_v<TIn*, T*>>>
	Span(const Span<TIn>& other)
		: m_data(other.Data()), m_size(other.Size())
	{
	}

	T* Data() const
	{
		return m_data;
	}

	iword Size() const
	{
		return m_size;
	}

	T& operator[](iword index) const
	{
		Assert(index >= 0 && index < m_size);
		return m_data[index];
	}

	void RemovePrefix(iword count)
	{
		Assert(count <= m_size);
		m_data += count;
		m_size -= count;
	}

	void RemoveSuffix(iword count)
	{
		Assert(count <= m_size);
		m_size -= count;
	}

	Span<T> Slice(i32 index) const
	{
		Assert(index < m_size);
		return Span<T>(m_data + index, m_size - index);
	}

	Span<T> Slice(i32 index, i32 count) const
	{
		Assert(index + count <= m_size);
		return Span<T>(m_data + index, count);
	}

private:
	T*   m_data;
	iword m_size;

	friend T* begin(const Span& span)
	{
		return span.m_data;
	}

	friend T* end(const Span& span)
	{
		return span.m_data + span.m_size;
	}
};

} // namespace
