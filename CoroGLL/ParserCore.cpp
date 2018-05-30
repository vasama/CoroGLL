#include "ParserCore.hpp"

#include "Core/Storage.hpp"

#include <exception>
#include <vector>

#include <experimental/coroutine>

namespace stdx = std::experimental;

using namespace CoroGLL;
using namespace CoroGLL::Private;
using namespace CoroGLL::Private::ParserCore;

namespace {

struct FrameFork;

struct Frame
{
	enum class State
	{
		None,
		Error,
		Ready,
	};

	Frame(i32 tokenIndex, ParseInfo parseInfo, std::size_t coroSize)
		: m_tokenIndex(tokenIndex), m_parseInfo(std::move(parseInfo)), m_coroSize(coroSize)
	{
	}

	auto GetForkIterator(FrameFork* fork)
	{
		for (auto first = m_forks.begin(), last = m_forks.end();; ++first)
		{
			Assert(first != last);
			if (*first == fork)
				return first;
		}
	}

	i32 FindFork(FrameFork* fork)
	{
		return GetForkIterator(fork) - m_forks.begin();
	}

	void RemoveFork(FrameFork* fork)
	{
		m_forks.erase(GetForkIterator(fork));
	}

	i32 m_tokenIndex;
	ParseInfo m_parseInfo;
	std::size_t m_coroSize;

	State m_state = State::None;

	FrameFork* m_error = nullptr;
	FrameFork* m_ready = nullptr;

	std::vector<FrameFork*> m_forks;
	std::vector<FrameFork*> m_dependants;

	union {
		struct {
			Ast::Syntax* syntax;
			i32 tokenIndex;
		};
		FrameFork* errorFork;
	} m_value;
};

struct FrameFork
{
	enum class State
	{
		Queue,
		Parse,
		Error,
		Ready,
	};

	FrameFork(FrameFork* fork)
		: m_frame(fork->m_frame), m_tokenIndex(fork->m_tokenIndex), m_state(State::Queue)
	{
		// copy the coroutine frame. sue me.
		m_coroBuffer = std::malloc(m_frame->m_coroSize);
		std::memcpy(m_coroBuffer, fork->m_coroBuffer, m_frame->m_coroSize);

		std::ptrdiff_t offset = (char*)fork->m_coro.address() - (char*)fork->m_coroBuffer;
		m_coro = stdx::coroutine_handle<>::from_address((char*)m_coroBuffer + offset);
	}

	FrameFork(Frame* frame, void* coroBuffer, stdx::coroutine_handle<> coro)
		: m_frame(frame), m_tokenIndex(frame->m_tokenIndex), m_state(State::Queue), m_coroBuffer(coroBuffer), m_coro(coro)
	{
	}

	~FrameFork()
	{
		std::free(m_coroBuffer);
	}

	Frame* m_frame;
	i32 m_tokenIndex;

	void* m_coroBuffer;
	stdx::coroutine_handle<> m_coro;

	State m_state;

	union {
		i32 forkIndex;
		Ast::Syntax* syntax;
		Frame* dependency;
	} m_value;

	FrameFork* m_errorFork;
};

class ParseContextImpl : ParseContext
{
	enum class State
	{
		None,

		Enter,
		Resume,
		Terminate,

		Suspend_Fork,
		Suspend_Parse,
		Suspend_Error,

		Exit_Ready,
		Exit_Throw,
	};

public:
	ParseContextImpl(Span<Ast::Token* const> tokens, SyntaxTreeContext* treeContext)
	{
		m_tokens = tokens;
		m_treeContext = treeContext;

		m_tokenIndex = 0;
	}


	Frame* CreateFrame(i32 tokenIndex, ParseInfo parseInfo)
	{
		Frame* frame = (Frame*)operator new(sizeof(Frame));
		CreateFrame(frame, tokenIndex, std::move(parseInfo));
		return frame;
	}

	void CreateFrame(Frame* frame, i32 tokenIndex, ParseInfo parseInfo)
	{
		Assert(std::exchange(m_state, State::Enter) == State::None);
		Promise* promise = parseInfo.Execute(this);

		new (frame) Frame(tokenIndex, std::move(parseInfo), m_value.coro.size);
		FrameFork* fork = new FrameFork(frame, m_value.coro.buffer, promise->GetCoro());
		frame->m_forks.push_back(fork);

		promise->SetContext(this);
	}


	enum class ResumeResult
	{
		Fork  = (i32)State::Suspend_Fork,
		Parse = (i32)State::Suspend_Parse,
		Error = (i32)State::Suspend_Error,
		Ready = (i32)State::Exit_Ready,
	};

	ResumeResult Resume(FrameFork* fork)
	{
		Assert(fork->m_state == FrameFork::State::Queue);
		Assert(std::exchange(m_state, State::Resume) == State::None);

		m_tokenIndex = fork->m_tokenIndex;

		m_value.fork = fork;
		fork->m_coro.resume();

		fork->m_tokenIndex = m_tokenIndex;

		State state = m_state;

#if DEBUG
		switch (state)
		{
		case State::Exit_Ready:
		case State::Suspend_Fork:
		case State::Suspend_Parse:
		case State::Suspend_Error:
			break;

		default:
			Assert(false);
		}
#endif

		return (ResumeResult)state;
	}

	i32 TakeForkCount()
	{
		Assert(std::exchange(m_state, State::None) == State::Suspend_Fork);
		return m_value.forkCount;
	}

	ParseInfo TakeParseInfo()
	{
		Assert(std::exchange(m_state, State::None) == State::Suspend_Parse);
		ParseInfo parseInfo(std::move(*m_value.parseInfo));
		m_value.parseInfo.Destroy();
		return parseInfo;
	}

	ErrorInfo TakeErrorInfo()
	{
		Assert(std::exchange(m_state, State::None) == State::Suspend_Error);
		ErrorInfo errorInfo(std::move(*m_value.errorInfo));
		m_value.errorInfo.Destroy();
		return errorInfo;
	}

	Ast::Syntax* TakeSyntax()
	{
		Assert(std::exchange(m_state, State::None) == State::Exit_Ready);
		return m_value.syntax;
	}


	void TerminateFork(FrameFork* fork)
	{
		m_state = State::Terminate;
		fork->m_coro.resume();

		Assert(std::exchange(m_state, State::None) == State::Exit_Throw);
	}

private:
	class TerminateException
	{
	};

	void OnSuspend(State newState)
	{
	//	Assert(m_state == State::None);
		m_state = newState;
	}

	void OnResume()
	{
		if (m_state == State::Terminate)
			throw TerminateException();

		Assert(std::exchange(m_state, State::None) == State::Resume);
	}

	State m_state = State::None;

	union {
		struct {
			std::size_t size;
			void* buffer;
		} coro;

		Frame* frame;
		FrameFork* fork;

		i32 forkCount;
		Storage<ParseInfo> parseInfo;
		Storage<ErrorInfo> errorInfo;

		Ast::Syntax* syntax;
	} m_value;

	friend class ParseContextCore;
};

typedef ParseContextImpl::ResumeResult ResumeResult;

class Parser
{
public:
	Parser(Span<Ast::Token*> tokens, SyntaxTreeContext* treeContext)
		: m_ctx(tokens, treeContext)
	{
	}

	Ast::Syntax* Parse(ParseInfo parseInfo)
	{
		Storage<Frame> root;
		m_ctx.CreateFrame(&root, 0, std::move(parseInfo));

		m_root = &root;

		Ast::Syntax* syntax = ParseCore();
		m_frames.clear();
		return syntax;
	}

private:
	Ast::Syntax* ParseCore()
	{
		while (true)
		{
			FrameFork* fork = FindLeastAdvancedLeaf();

			if (m_frames.size() > 0)
			{
				i32 tokenIndex = fork->m_tokenIndex;

				auto first = m_frames.begin();
				decltype(first) it = first;

				for (decltype(first) last = m_frames.begin();
					it != last && (*it)->m_tokenIndex < tokenIndex; ++it);

				m_frames.erase(first, it);
			}

			HandleResult handleResult = HandleResult::None;

			switch (m_ctx.Resume(fork))
			{
			case ResumeResult::Fork:
				{
					i32 forkCount = m_ctx.TakeForkCount();

					Frame* frame = fork->m_frame;
					i32 slotIndex = frame->FindFork(fork) + 1;

					frame->m_forks.reserve(frame->m_forks.size() + forkCount - 1);
					for (i32 forkIndex = 1; forkIndex < forkCount; ++forkIndex)
					{
						FrameFork* newFork = new FrameFork(fork);
						newFork->m_value.forkIndex = forkIndex;

						frame->m_forks.insert(frame->m_forks.begin() + slotIndex++, newFork);
					}
					fork->m_value.forkIndex = 0;
				}
				break;

			case ResumeResult::Parse:
				{
					Frame* dependency = FindOrCreateFrame(fork->m_tokenIndex, m_ctx.TakeParseInfo());

					switch (dependency->m_state)
					{
					case Frame::State::None:
						fork->m_value.dependency = dependency;
						dependency->m_dependants.push_back(fork);
						fork->m_state = FrameFork::State::Parse;
						break;

					case Frame::State::Error:
						handleResult = HandleError(fork, dependency->m_value.errorFork);
						break;

					case Frame::State::Ready:
						fork->m_tokenIndex = dependency->m_value.tokenIndex;
						fork->m_value.syntax = dependency->m_value.syntax;
						break;
					}
				}
				break;

			case ResumeResult::Error:
				m_ctx.TakeErrorInfo();
				handleResult = HandleError(fork, fork);
				break;

			case ResumeResult::Ready:
				handleResult = HandleReady(fork, m_ctx.TakeSyntax());
				break;
			}

			switch (handleResult)
			{
			case HandleResult::Error:
				//TODO: commit temporary syntax
				SwallowErrors(m_root);
				break;

			case HandleResult::Ready:
				return m_root->m_value.syntax;
			}
		}
	}

	FrameFork* FindLeastAdvancedLeaf()
	{
		return FindLeastAdvancedLeaf(m_root);
	}

	FrameFork* FindLeastAdvancedLeaf(Frame* frame)
	{
		FrameFork* candidate = nullptr;

		for (FrameFork* fork : frame->m_forks)
		{
			switch (fork->m_state)
			{
			case FrameFork::State::Parse:
				fork = FindLeastAdvancedLeaf(fork->m_value.dependency);

			case FrameFork::State::Queue:
				if (!candidate || fork->m_tokenIndex < candidate->m_tokenIndex)
					candidate = fork;
			}
		}

		Assert(candidate);
		return candidate;
	}

	Frame* FindOrCreateFrame(i32 tokenIndex, ParseInfo parseInfo)
	{
		auto iterator = m_frames.begin();
		for (decltype(iterator) end = m_frames.end(); iterator != end; ++iterator)
		{
			Frame* frame = *iterator;

			if (frame->m_tokenIndex == tokenIndex && frame->m_parseInfo == parseInfo)
				return frame;

			if (frame->m_tokenIndex > tokenIndex)
				break;
		}

		Frame* frame = m_ctx.CreateFrame(tokenIndex, std::move(parseInfo));
		m_frames.insert(iterator, frame);
		return frame;
	}

	enum class HandleResult
	{
		None,
		Error,
		Ready,
	};

	HandleResult HandleError(FrameFork* fork, FrameFork* errorFork)
	{
		if (fork->m_state == FrameFork::State::Queue)
			fork->m_state = FrameFork::State::Error;
		fork->m_errorFork = errorFork;

		Frame* frame = fork->m_frame;

		if (frame->m_forks.size() == 1)
			return HandleError(frame, errorFork);

		if (FrameFork* ready = frame->m_ready)
		{
			frame->RemoveFork(fork);
			m_ctx.TerminateFork(fork);
			delete fork;

			if (frame->m_forks.size() == 1)
				return HandleReady(frame, ready->m_value.syntax);
		}
		else if (FrameFork* error = frame->m_error)
		{
			{
				i32 tokenIndex = errorFork->m_tokenIndex;
				i32 otherTokenIndex = error->m_errorFork->m_tokenIndex;

				if (tokenIndex == otherTokenIndex ? IsBetter(fork, error) : tokenIndex > otherTokenIndex)
					frame->m_error = fork;
				else
					std::swap(fork, error);
			}

			frame->RemoveFork(fork);
			m_ctx.TerminateFork(fork);
			delete fork;

			if (frame->m_forks.size() == 1)
				return HandleError(frame, error->m_errorFork);
		}
		else
		{
			frame->m_error = fork;
		}

		return HandleResult::None;
	}

	HandleResult HandleReady(FrameFork* fork, Ast::Syntax* syntax)
	{
		fork->m_state = FrameFork::State::Ready;
		fork->m_value.syntax = syntax;

		Frame* frame = fork->m_frame;

		if (frame->m_forks.size() == 1)
			return HandleReady(frame, syntax);

		if (FrameFork* ready = frame->m_ready)
		{
			//the newly ready fork must be better

			frame->RemoveFork(ready);
			delete ready;
		}
		else if (FrameFork* error = frame->m_error)
		{
			frame->RemoveFork(error);
			m_ctx.TerminateFork(error);
			delete error;

			frame->m_error = nullptr;
		}
		frame->m_ready = fork;

		//delete all worse forks
		{
			auto first = frame->GetForkIterator(fork) + 1;
			decltype(first) last = frame->m_forks.end();

			if (first != last)
			{
				auto it = first;

				do
				{
					FrameFork* x = *it++;
					m_ctx.TerminateFork(x);
					delete x;
				} while (it != last);

				frame->m_forks.erase(first, last);
			}
		}

		if (frame->m_forks.size() == 1)
			return HandleReady(frame, syntax);

		return HandleResult::None;
	}
	
	HandleResult HandleError(Frame* frame, FrameFork* errorFork)
	{
		frame->m_state = Frame::State::Error;
		frame->m_value.errorFork = errorFork;

		if (frame == m_root)
			return HandleResult::Error;

		//TODO: temporarily take shared ownership of the frame

		HandleResult result = HandleResult::None;
		for (FrameFork* fork : frame->m_dependants)
			result = (HandleResult)((i32)result | (i32)HandleError(fork, errorFork));

		return result;
	}

	HandleResult HandleReady(Frame* frame, Ast::Syntax* syntax)
	{
		Assert(frame->m_forks.size() == 1);
		FrameFork* fork = frame->m_forks[0];
		i32 tokenIndex = fork->m_tokenIndex;

		frame->m_state = Frame::State::Ready;
		frame->m_tokenIndex = tokenIndex;
		frame->m_value.syntax = syntax;

		if (frame == m_root)
			return HandleResult::Ready;

		//TODO: temporarily take shared ownership of the frame

		for (FrameFork* dependant : frame->m_dependants)
		{
			dependant->m_state = FrameFork::State::Queue;
			dependant->m_tokenIndex = tokenIndex;
			dependant->m_value.syntax = syntax;
		}
		frame->m_dependants.clear();

		return HandleResult::None;
	}

	void SwallowErrors(Frame* frame)
	{
		Assert(frame->m_state == Frame::State::Error);
		frame->m_state = Frame::State::None;

		Assert(frame->m_forks.size() == 1);
		FrameFork* fork = frame->m_forks[0];

		switch (fork->m_state)
		{
		case FrameFork::State::Parse:
			SwallowErrors(fork->m_value.dependency);
			break;

		case FrameFork::State::Error:
			fork->m_state = FrameFork::State::Queue;
			break;

		default:
			Assert(false);
		}
	}

	bool IsBetter(FrameFork* a, FrameFork* b)
	{
		Frame* frame = a->m_frame;
		Assert(frame == b->m_frame);

		return frame->FindFork(a) < frame->FindFork(b);
	}

	ParseContextImpl m_ctx;

	std::vector<Frame*> m_frames;
	Frame* m_root;
};

} // namespace

#define this ( static_cast<ParseContextImpl*>(this) )

void* ParseContextCore::Enter(std::size_t coroSize)
{
	typedef ParseContextImpl::State State;
	Assert(std::exchange(this->m_state, State::None) == State::Enter);

	void* buffer = std::malloc(coroSize);
	this->m_value.coro.size = coroSize;
	this->m_value.coro.buffer = buffer;

	return buffer;
}

void ParseContextCore::Suspend_Fork(i32 forkCount)
{
	this->OnSuspend(ParseContextImpl::State::Suspend_Fork);
	this->m_value.forkCount = forkCount;
}

i32 ParseContextCore::Resume_Fork()
{
	this->OnResume();
	return this->m_value.fork->m_value.forkIndex;
}

void ParseContextCore::Suspend_Parse(ParseInfo parseInfo)
{
	this->OnSuspend(ParseContextImpl::State::Suspend_Parse);
	this->m_value.parseInfo.Construct(std::move(parseInfo));
}

Ast::Syntax* ParseContextCore::Resume_Parse()
{
	this->OnResume();
	return this->m_value.fork->m_value.syntax;
}

void ParseContextCore::Suspend_Error(ErrorInfo errorInfo)
{
	this->OnSuspend(ParseContextImpl::State::Suspend_Error);
	this->m_value.errorInfo.Construct(std::move(errorInfo));
}

void ParseContextCore::Resume_Error()
{
	this->OnResume();
}

void ParseContextCore::Exit_Ready(Ast::Syntax* syntax)
{
	this->OnSuspend(ParseContextImpl::State::Exit_Ready);
	this->m_value.syntax = syntax;
}

void ParseContextCore::Exit_Throw()
{
	typedef ParseContextImpl::State State;
	Assert(std::exchange(this->m_state, State::Exit_Throw) == State::Terminate);

#if DEBUG
	try
	{
		std::rethrow_exception(std::current_exception());
	}
	catch (ParseContextImpl::TerminateException&)
	{
	}
#endif
}

#undef this

Ast::Syntax* CoroGLL::Private::ParserCore::ParseCore(Span<Ast::Token*> tokens, SyntaxTreeContext* treeContext, ParseInfo parseInfo)
{
	return Parser(tokens, treeContext).Parse(std::move(parseInfo));
}
