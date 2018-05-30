#include "SyntaxKind.hpp"

#include <string_view>

namespace
{
	const std::string_view SyntaxKindString[] = {
		"None",

#define COROGLL_X(n) #n "Trivia" ,
		COROGLL_TRIVIA(COROGLL_X)
#undef COROGLL_X

#define COROGLL_X(n) #n "Token" ,
		COROGLL_TOKEN(COROGLL_X)
#undef COROGLL_X

#define COROGLL_X(n) #n "Symbol" ,
		COROGLL_SYMBOL(COROGLL_X)
#undef COROGLL_X

#define COROGLL_X(n, s) #n "Keyword" ,
		COROGLL_KEYWORD(COROGLL_X)
#undef COROGLL_X

#define COROGLL_X(n) #n "Expression" ,
		COROGLL_EXPRESSION(COROGLL_X)
		COROGLL_UNARY_OPERATOR(COROGLL_X)
		COROGLL_BINARY_OPERATOR(COROGLL_X)
		COROGLL_INVOKE_OPERATOR(COROGLL_X)
#undef COROGLL_X

#define COROGLL_X(n) #n "AccessExpression" ,
		COROGLL_ACCESS_OPERATOR(COROGLL_X)
#undef COROGLL_X

#define COROGLL_X(n) #n ,
		COROGLL_SYNTAX(COROGLL_X)
#undef COROGLL_X
	};
}

std::string_view CoroGLL::Ast::ToString(Ast::SyntaxKind syntaxKind)
{
	return SyntaxKindString[(uword)syntaxKind];
}