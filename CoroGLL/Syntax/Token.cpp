#include "Token.hpp"
#include "../Core/Types.hpp"

#include <string_view>

namespace
{
	using namespace CoroGLL::Ast;
	void PrintType(std::ostream& stream, const WordToken* type)
	{
		if (!type)
			return;
		if (type->Kind() == SyntaxKind::NameToken && static_cast<const NameToken*>(type)->Verbatim())
			stream << '@';
		stream << type->String();
	}
}

std::string_view CoroGLL::Ast::ToString(Keyword keyword)
{
	switch (keyword)
	{
#define COROGLL_X(n, s) case Keyword::n: return s;
		COROGLL_KEYWORD(COROGLL_X)
#undef COROGLL_X
	}

	//TODO: fix MSVC only
	__assume(0);
}

bool CoroGLL::Ast::ResolveKeyword(std::string_view string, Keyword& out)
{
	//TODO: efficient search. radix tree?
#define COROGLL_X(n, s) if (string == s) { out = Keyword::n; return true; }
	COROGLL_KEYWORD(COROGLL_X);
#undef COROGLL_X
	return false;
}
