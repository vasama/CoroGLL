#pragma once

#include "Core/Debug.hpp"
#include "Core/Span.hpp"
#include "Core/Types.hpp"
#include "Syntax/Syntax.hpp"
#include "Syntax/Token.hpp"
#include "SyntaxTree.hpp"

#include <tuple>
#include <utility>

#include <experimental/coroutine>

namespace CoroGLL::Private::ParserCore {

class ParseContext;
class ParseContextCore;

class Promise
{
public:
	std::experimental::coroutine_handle<> GetCoro() const
	{
		Assert(m_state == 1);
		return std::experimental::coroutine_handle<>::from_address(m_coro);
	}

	void SetCoro(std::experimental::coroutine_handle<> coro)
	{
		Assert(std::exchange(m_state, 1) == 0);
		m_coro = coro.address();
	}

	ParseContextCore* GetContext() const
	{
		Assert(m_state == 2);
		return m_context;
	}

	void SetContext(ParseContextCore* context)
	{
		Assert(std::exchange(m_state, 2) == 1);
		m_context = context;
	}

private:
	union {
		void* m_coro;
		ParseContextCore* m_context;
	};

#if DEBUG
	i32 m_state = 0;
#endif
};

template<typename TSyntax>
class Result
{
public:
	Result(Promise* promise)
		: m_promise(promise)
	{
	}

	Promise* GetPromise() const
	{
		return m_promise;
	}

private:
	Promise* m_promise;
};

class ParseContext;

class ParseInfo
{
	class Interface
	{
	public:
		bool Equals(const Interface& other) const
		{
			return m_func == other.m_func && DoEquals(other);
		}

		virtual Promise* Execute(ParseContext* context) = 0;

	protected:
		Interface(void(*func)())
			: m_func(func)
		{
		}

		virtual bool DoEquals(const Interface& other) const = 0;

		void(*m_func)();
	};

	template<typename TSyntax, typename... TParams>
	class Implementation : public Interface
	{
	public:
		template<typename... TArgs>
		Implementation(Result<TSyntax>(*func)(ParseContext*, TParams...), TArgs&&... args)
			: Interface((void(*)())func), m_args(std::forward<TArgs>(args)...)
		{
		}

		virtual Promise* Execute(ParseContext* context) override
		{
			return ExecuteInternal(context, (Result<TSyntax>(*)(ParseContext*, TParams...))m_func, std::index_sequence_for<TParams...>());
		}

		virtual bool DoEquals(const Interface& other) const override
		{
			return m_args == static_cast<const Implementation<TSyntax, TParams...>&>(other).m_args;
		}

	private:
		template<std::size_t... TIndices>
		Promise* ExecuteInternal(ParseContext* context, Result<TSyntax>(*func)(ParseContext*, TParams...), std::index_sequence<TIndices...>)
		{
			Assert(std::exchange(m_exec, true) == false);
			return func(context, std::get<TIndices>(m_args)...).GetPromise();
		}

		std::tuple<TParams...> m_args;

#if DEBUG
		bool m_exec = false;
#endif
	};

public:
	template<typename TSyntax, typename... TParams, typename... TArgs>
	ParseInfo(Result<TSyntax>(*func)(ParseContext*, TParams...), TArgs&&... args)
		: m_impl(new Implementation<TSyntax, TParams...>(func, std::forward<TArgs>(args)...))
	{
	}

	Promise* Execute(ParseContext* context) const
	{
		return m_impl->Execute(context);
	}

	bool operator==(const ParseInfo& other) const
	{
		return m_impl == other.m_impl || m_impl->Equals(*other.m_impl);
	}

	bool operator!=(const ParseInfo& other) const
	{
		return m_impl != other.m_impl && !m_impl->Equals(*other.m_impl);
	}

private:
	std::shared_ptr<Interface> m_impl;
};

class ErrorInfo
{
};

struct ParseContextAttorney;

template<typename TAwaiter>
class Future
{
	Future(ParseContextCore* ctx)
		: m_ctx(ctx)
	{
	}

	ParseContextCore* m_ctx;

	friend struct FutureAttorney;
};

struct FutureAttorney
{
	template<typename TAwaiter>
	static Future<TAwaiter> CreateFuture(ParseContextCore* ctx)
	{
		return Future<TAwaiter>(ctx);
	}

	template<typename TAwaiter>
	static ParseContextCore* GetContext(const Future<TAwaiter>& future)
	{
		return future.m_ctx;
	}
};

class AwaiterCore
{
public:
	bool await_ready()
	{
		return false;
	}

	void await_suspend(std::experimental::coroutine_handle<>)
	{
	}

protected:
	AwaiterCore(ParseContextCore* ctx)
		: m_ctx(ctx)
	{
	}

	ParseContextCore* GetContext() const
	{
		return m_ctx;
	}

private:
	ParseContextCore * m_ctx;
};

template<typename TAwaiter>
TAwaiter operator co_await(const Future<TAwaiter>& future)
{
	return TAwaiter(FutureAttorney::GetContext(future));
}

class ParseContextCore
{
public:
	void* Enter(std::size_t coroSize);

	void Suspend_Fork(i32 forkCount);
	i32 Resume_Fork();

	void Suspend_Parse(ParseInfo parseInfo);
	Ast::Syntax* Resume_Parse();

	void Suspend_Error(ErrorInfo errorInfo);
	void Resume_Error();

	void Exit_Ready(Ast::Syntax* syntax);
	void Exit_Throw();

	static ParseContextCore* GetCore(ParseContext* context);

protected:
	ParseContextCore()
	{
	}
};

class ForkAwaiter : public AwaiterCore
{
public:
	ForkAwaiter(ParseContextCore* ctx)
		: AwaiterCore(ctx)
	{
	}

	[[nodiscard]] i32 await_resume()
	{
		return GetContext()->Resume_Fork();
	}
};

template<typename TSyntax>
class ParseAwaiter : public AwaiterCore
{
public:
	ParseAwaiter(ParseContextCore* ctx)
		: AwaiterCore(ctx)
	{
	}

	[[nodiscard]] TSyntax* await_resume()
	{
		Ast::Syntax* syntax = GetContext()->Resume_Parse();
		return static_cast<TSyntax*>(syntax);
	}
};

class ErrorAwaiter : public AwaiterCore
{
public:
	ErrorAwaiter(ParseContextCore* ctx)
		: AwaiterCore(ctx)
	{
	}

	void await_resume()
	{
		GetContext()->Resume_Error();
	}
};

class ParseContext : protected ParseContextCore
{
public:
	[[nodiscard]] Ast::Token* PeekToken(i32 index = 0)
	{
		Assert(m_tokenIndex + index < m_tokens.Size());
		return m_tokens[m_tokenIndex + index];
	}

	[[nodiscard]] Ast::Token* EatToken()
	{
		Assert(m_tokenIndex < m_tokens.Size());
		return m_tokens[m_tokenIndex++];
	}

	template<typename TSyntax, typename... TArgs>
	[[nodiscard]] std::enable_if_t<std::is_base_of_v<Ast::Syntax, TSyntax>, TSyntax*> CreateSyntax(TArgs&&... args)
	{
		return m_treeContext->CreateSyntax<TSyntax>(std::forward<TArgs>(args)...);
	}

	template<typename TSyntax, typename TIterator>
	[[nodiscard]] std::enable_if_t<std::is_base_of_v<Ast::Syntax, TSyntax>, Span<TSyntax*>> CreateSyntaxList(TIterator first, TIterator last)
	{
		return m_treeContext->CreateSyntaxList<TSyntax>(first, last);
	}

	[[nodiscard]] Future<ForkAwaiter> Fork(i32 forkCount)
	{
		Assert(forkCount > 1);
		Suspend_Fork(forkCount);

		return FutureAttorney::CreateFuture<ForkAwaiter>(this);
	}

	template<typename TSyntax, typename... TParams, typename... TArgs>
	[[nodiscard]] Future<ParseAwaiter<TSyntax>> Parse(Result<TSyntax>(*func)(ParseContext*, TParams...), TArgs&&... args)
	{
		Suspend_Parse(ParseInfo(func, std::forward<TArgs>(args)...));

		return FutureAttorney::CreateFuture<ParseAwaiter<TSyntax>>(this);
	}

	[[nodiscard]] Future<ErrorAwaiter> SetError()
	{
		Suspend_Error(ErrorInfo());

		return FutureAttorney::CreateFuture<ErrorAwaiter>(this);
	}

protected:
	ParseContext()
	{
	}

	i32 m_tokenIndex;
	Span<Ast::Token* const> m_tokens;
	SyntaxTreeContext* m_treeContext;

	friend class ParseContextCore;
};

inline ParseContextCore* ParseContextCore::GetCore(ParseContext* context)
{
	return context;
}

Ast::Syntax* ParseCore(Span<Ast::Token*> tokens, SyntaxTreeContext* treeContext, ParseInfo parseInfo);

template<typename TSyntax, typename... TParams, typename... TArgs>
TSyntax* Parse(Span<Ast::Token*> tokens, SyntaxTreeContext* treeContext, Result<TSyntax>(*func)(ParseContext*, TParams...), TArgs&&... args)
{
	return static_cast<TSyntax*>(ParseCore(tokens, treeContext, ParseInfo(func, std::forward<TArgs>(args)...)));
}

} // namespace CoroGLL::ParserCore

template<typename TSyntax, typename... TParams>
struct std::experimental::coroutine_traits<CoroGLL::Private::ParserCore::Result<TSyntax>,
	CoroGLL::Private::ParserCore::ParseContext*, TParams...>
{
	struct promise_type
	{
		CoroGLL::Private::ParserCore::Promise m_promise;

		promise_type()
		{
			m_promise.SetCoro(std::experimental::coroutine_handle<promise_type>::from_promise(*this));
		}

		std::experimental::suspend_always initial_suspend()
		{
			return std::experimental::suspend_always();
		}

		std::experimental::suspend_never final_suspend()
		{
			return std::experimental::suspend_never();
		}

		CoroGLL::Private::ParserCore::Result<TSyntax> get_return_object()
		{
			return CoroGLL::Private::ParserCore::Result<TSyntax>(&m_promise);
		}

		void return_value(TSyntax* syntax)
		{
			m_promise.GetContext()->Exit_Ready(syntax);
		}

		void unhandled_exception()
		{
			m_promise.GetContext()->Exit_Throw();
		}

		void* operator new(std::size_t coroSize, CoroGLL::Private::ParserCore::ParseContext* ctx, const TParams&...)
		{
			return CoroGLL::Private::ParserCore::ParseContextCore::GetCore(ctx)->Enter(coroSize);
		}

		void operator delete(void* coro, std::size_t coroSize)
		{
		}
	};
};
