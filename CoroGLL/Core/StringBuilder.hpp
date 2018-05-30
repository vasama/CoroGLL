#pragma once

#include "Types.hpp"

#include <vector>

namespace CoroGLL {

template<typename TChar>
class BasicStringBuilder
{
public:
	BasicStringBuilder()
	{
	}

	iword Size() const
	{
		return m_size;
	}

	iword CopyTo(TChar* data, iword size) const
	{
		iword total = size;
		for (std::string_view x : m_vector)
		{
			iword s = x.copy(data, size);
			if (s < size) break;
			data += s;
			size -= s;
		}
		return total - size;
	}

	BasicStringBuilder& operator+=(std::string_view string)
	{
		if (!string.empty())
		{
			m_vector.push_back(string);
			m_size += string.size();
		}
		return *this;
	}

	BasicStringBuilder& operator+=(const BasicStringBuilder& other)
	{
		if (other.m_size > 0)
		{
			m_vector.insert(m_vector.end(), other.m_vector.begin(), other.m_vector.end());
			m_size += other.m_size;
		}
		return *this;
	}

private:
	std::vector<std::string_view> m_vector;
	iword m_size = 0;
};

typedef BasicStringBuilder<char> StringBuilder;

} // namespace CoroGLL
