#include "SyntaxTree.hpp"

#include "Core/Math.hpp"
#include "Core/Types.hpp"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <new>

namespace {

using namespace CoroGLL;

void* BlockAlloc(uword size)
{
	return std::malloc(size);
}

void BlockFree(void* block, uword size)
{
	(void)size;
	std::free(block);
}

struct RefCountBase
{
	std::atomic<iword> RefCount;
};

} // namespace

struct CoroGLL::Private::SyntaxTreeContext::Block
{
	Block* Next;

	void* Data;
	void* Last;

	Block(uword size)
		: Next(nullptr), Data(this + 1), Last((char*)(this + 1) + size)
	{

	}

	uword TotalSize() const
	{
		return (char*)Last - (char*)(this + 1);
	}

	uword SpaceLeft() const
	{
		return (char*)Last - (char*)Data;
	}
};

struct CoroGLL::Private::SyntaxTreeContext::First : RefCountBase, Block
{
	First(uword size)
		: Block(size)
	{
	}
};

CoroGLL::Private::SyntaxTreeContext::SyntaxTreeContext()
	: m_head(nullptr)
{
}

CoroGLL::Private::SyntaxTreeContext::~SyntaxTreeContext()
{
	if (m_head && std::atomic_fetch_sub_explicit(&m_head->RefCount, 1, std::memory_order_relaxed) == 0)
		Clean(m_head);
}

CoroGLL::Private::SyntaxTreeContext::SyntaxTreeContext(SyntaxTreeContext&& other)
	: m_head(other.m_head), m_tail(other.m_tail)
{
	other.m_head = nullptr;
}

CoroGLL::Private::SyntaxTreeContext& CoroGLL::Private::SyntaxTreeContext::operator=(SyntaxTreeContext&& other)
{
	if (m_head && std::atomic_fetch_sub_explicit(&m_head->RefCount, 1, std::memory_order_relaxed) == 0)
		Clean(m_head);

	m_head = other.m_head;
	m_tail = other.m_tail;

	other.m_head = nullptr;

	return *this;
}

CoroGLL::Private::SyntaxTreeContext::SyntaxTreeContext(const SyntaxTreeContext& other)
	: m_head(other.m_head), m_tail(other.m_tail)
{
	if (m_head) std::atomic_fetch_add_explicit(&m_head->RefCount, 1, std::memory_order_relaxed);
}

CoroGLL::Private::SyntaxTreeContext& CoroGLL::Private::SyntaxTreeContext::operator=(const SyntaxTreeContext& other)
{
	if (m_head && std::atomic_fetch_sub_explicit(&m_head->RefCount, 1, std::memory_order_relaxed) == 0)
		Clean(m_head);

	m_head = other.m_head;
	m_tail = other.m_tail;

	if (m_head) std::atomic_fetch_add_explicit(&m_head->RefCount, 1, std::memory_order_relaxed);

	return *this;
}

std::string_view CoroGLL::Private::SyntaxTreeContext::CreateString(std::string_view string)
{
	char* buffer = (char*)Allocate(string.size(), alignof(char));
	string.copy(buffer, string.size());

	return std::string_view(buffer, string.size());
}

std::string_view CoroGLL::Private::SyntaxTreeContext::CreateString(const StringBuilder& stringBuilder)
{
	char* buffer = (char*)Allocate(stringBuilder.Size(), alignof(char));
	stringBuilder.CopyTo(buffer, stringBuilder.Size());

	return std::string_view(buffer, stringBuilder.Size());
}

void CoroGLL::Private::SyntaxTreeContext::Reset()
{
	if (Block* block = m_head)
	{
		block->Data = block + 1;
		m_tail = block;
	}
}

void CoroGLL::Private::SyntaxTreeContext::Clean(First* first)
{
	Block* block = first->Next;
	BlockFree(first, sizeof(First) + first->TotalSize());

	while (block)
	{
		Block* next = block->Next;
		BlockFree(block, sizeof(Block) + block->TotalSize());

		block = next;
	}
}

void* CoroGLL::Private::SyntaxTreeContext::Allocate(uword size, uword align)
{
	//HACK: something is broken
	return std::malloc(size);

	if (!m_head)
	{
		uword blockSize = size + sizeof(First);
		blockSize = NextPowerOfTwo(blockSize);

		First* first = new (BlockAlloc(blockSize)) First(blockSize - sizeof(First));

		m_head = first;
		m_tail = first;
	}
	else if (m_tail->SpaceLeft() < size)
	{
		uword blockSize = size + sizeof(Block);
		blockSize = NextPowerOfTwo(blockSize);

		Block* block = new (BlockAlloc(blockSize)) Block(blockSize - sizeof(First));

		m_tail->Next = block;
		m_tail = block;
	}

	void* data = m_tail->Data;
	m_tail->Data = (char*)data + size;
	return data;
}
