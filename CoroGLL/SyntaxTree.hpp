#pragma once

#include "Core/Span.hpp"
#include "Core/StringBuilder.hpp"
#include "Core/Types.hpp"
#include "Syntax/Syntax.hpp"

#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace CoroGLL::Private {

class SyntaxTreeContext
{
public:
	SyntaxTreeContext();
	~SyntaxTreeContext();

	SyntaxTreeContext(SyntaxTreeContext&&);
	SyntaxTreeContext& operator=(SyntaxTreeContext&&);

	SyntaxTreeContext(const SyntaxTreeContext&);
	SyntaxTreeContext& operator=(const SyntaxTreeContext&);

	template<typename T, typename... TArgs>
	T* CreateSyntax(TArgs&&... args)
	{
		static_assert(std::is_base_of_v<Ast::Syntax, T>);

		return new (Allocate(sizeof(T), alignof(T))) T(std::forward<TArgs>(args)...);
	}

	template<typename T, typename TIterator>
	Span<T*> CreateSyntaxList(TIterator first, TIterator last)
	{
		static_assert(std::is_base_of_v<Ast::Syntax, T>);

		i32 size = std::distance(first, last);
		if (size <= 0) return Span<T*>();

		T** data = (T**)Allocate(sizeof(T*) * size, alignof(T*));

		for (T** out = data; first != last; ++first)
			*out++ = *first;

		return Span<T*>(data, size);
	}

	std::string_view CreateString(std::string_view string);
	std::string_view CreateString(const StringBuilder& stringBuilder);

	void Reset();

private:
	struct First;
	struct Block;

	void Clean(First* first);

	void* Allocate(uword size, uword align);

	First* m_head;
	Block* m_tail;
};

struct SyntaxTreeAttorney;

} // namespace Private

namespace CoroGLL {

class SyntaxTree
{
public:
	Ast::Syntax* GetRoot()
	{
		return m_root;
	}

private:
	SyntaxTree(Ast::Syntax* root, Private::SyntaxTreeContext ctx)
		: m_root(root), m_context(std::move(ctx))
	{
	}

	Ast::Syntax* m_root;
	Private::SyntaxTreeContext m_context;

	friend struct Private::SyntaxTreeAttorney;
};

} // namespace CoroGLL

struct CoroGLL::Private::SyntaxTreeAttorney
{
	static SyntaxTree CreateSyntaxTree(Ast::Syntax* root, SyntaxTreeContext&& ctx)
	{
		return SyntaxTree(root, std::move(ctx));
	}
};
