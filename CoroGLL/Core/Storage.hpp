#pragma once

namespace CoroGLL {

template<typename T>
class Storage
{
public:
	template<typename... TArgs>
	void Construct(TArgs&&... args)
	{
		::new (&m_storage) T(std::forward<TArgs>(args)...);
	}

	void Destroy()
	{
		((T*)&m_storage)->~T();
	}

	T& operator*()
	{
		return *(T*)&m_storage;
	}

	T* operator->()
	{
		return (T*)&m_storage;
	}

	T* operator&()
	{
		return (T*)&m_storage;
	}

private:
	std::aligned_storage_t<
		sizeof(T), alignof(T)
	> m_storage;
};

} // namespace CoroGLL
