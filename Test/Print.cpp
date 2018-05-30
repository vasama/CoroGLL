#include "Print.hpp"
#include "Syntax/Expression.hpp"

using namespace CoroGLL;
using namespace CoroGLL::Ast;

namespace {

void Print(std::ostream& os, WordToken* token)
{
	if (token->Kind() == SyntaxKind::NameToken && static_cast<NameToken*>(token)->Verbatim())
		os << '@';
	os << token->String();
}

void PrintIndent(std::ostream& os, i32 indent)
{
	for (i32 i = 0; i < indent; ++i)
		os << "| ";
}

void PrintSyntax(std::ostream& os, i32 indent, Syntax* syntax);

void PrintContent(std::ostream& os, i32 indent, Syntax* syntax)
{
	PrintIndent(os, indent);
	os << "?\n";
}

void PrintContent(std::ostream& os, i32 indent, CastExpression* syntax)
{
	PrintSyntax(os, indent, syntax->type);
	PrintSyntax(os, indent, syntax->expression);
}

void PrintContent(std::ostream& os, i32 indent, LiteralExpression* syntax)
{
	PrintIndent(os, indent);

	LiteralToken* token = syntax->literalToken;
	switch (token->Kind())
	{
	case SyntaxKind::CharLiteralToken:
		os << '\'' << static_cast<CharLiteralToken*>(token)->Value() << '\'';
		break;

	case SyntaxKind::StringLiteralToken:
		os << '\"' << static_cast<StringLiteralToken*>(token)->Value() << '\"';
		break;

	case SyntaxKind::NumericLiteralToken:
		os << static_cast<NumericLiteralToken*>(token)->Value();
		break;
	}

	if (WordToken* type = token->Type())
		Print(os, type);

	os << '\n';
}

void PrintContent(std::ostream& os, i32 indent, MetaExpression* syntax)
{
	PrintSyntax(os, indent, syntax->expression);
}

void PrintContent(std::ostream& os, i32 indent, ParenthesizedExpression* syntax)
{
	PrintSyntax(os, indent, syntax->expression);
}

void PrintContent(std::ostream& os, i32 indent, TernaryExpression* syntax)
{
	PrintSyntax(os, indent, syntax->condition);
	if (Expression* trueExpression = syntax->trueExpression)
		PrintSyntax(os, indent, trueExpression);
	PrintSyntax(os, indent, syntax->falseExpression);
}

void PrintContent(std::ostream& os, i32 indent, WordExpression* syntax)
{
	PrintIndent(os, indent);
	Print(os, syntax->nameToken);
	os << '\n';
}

void PrintContent(std::ostream& os, i32 indent, WildcardExpression* syntax)
{
	PrintSyntax(os, indent, syntax->expression);
}

void PrintContent(std::ostream& os, i32 indent, UnaryExpression* syntax)
{
	PrintSyntax(os, indent, syntax->expression);
}

void PrintContent(std::ostream& os, i32 indent, BinaryExpression* syntax)
{
	PrintSyntax(os, indent, syntax->leftExpression);
	PrintSyntax(os, indent, syntax->rightExpression);
}

void PrintContent(std::ostream& os, i32 indent, Argument* syntax)
{
	if (WordToken* name = syntax->nameToken)
	{
		PrintIndent(os, indent);
		Print(os, name);
		os << '\n';
	}
	PrintSyntax(os, indent, syntax->expression);
}

void PrintContent(std::ostream& os, i32 indent, ArgumentList* syntax)
{
	for (Argument* x : syntax->arguments)
		PrintSyntax(os, indent, x);
}

void PrintContent(std::ostream& os, i32 indent, InvokeExpression* syntax)
{
	PrintSyntax(os, indent, syntax->expression);
	PrintSyntax(os, indent, syntax->arguments);
}

void PrintContent(std::ostream& os, i32 indent, AccessExpression* syntax)
{
	PrintSyntax(os, indent, syntax->expression);
	PrintIndent(os, indent);
	Print(os, syntax->nameToken);
	os << '\n';
}

void PrintSyntax(std::ostream& os, i32 indent, Syntax* syntax)
{
	PrintIndent(os, indent);
	os << ToString(syntax->Kind()) << '\n';
//	PrintIndent(os, indent);
//	os << "(\n";
	i32 nextIndent = indent + 1;
	switch (syntax->Kind())
	{
#define COROGLL_X(n) case SyntaxKind::n: \
PrintContent(os, nextIndent, static_cast<n*>(syntax)); break;
	COROGLL_SYNTAX(COROGLL_X);
#undef COROGLL_X

#define COROGLL_X(n) case SyntaxKind::n ## Expression: \
PrintContent(os, nextIndent, static_cast<n ## Expression*>(syntax)); break;
	COROGLL_EXPRESSION(COROGLL_X);
#undef COROGLL_X

#define COROGLL_X(n) case SyntaxKind::n ## Expression:
	COROGLL_UNARY_OPERATOR(COROGLL_X);
#undef COROGLL_X
		PrintContent(os, nextIndent, static_cast<UnaryExpression*>(syntax));
		break;

#define COROGLL_X(n) case SyntaxKind::n ## Expression:
	COROGLL_BINARY_OPERATOR(COROGLL_X);
#undef COROGLL_X
		PrintContent(os, nextIndent, static_cast<BinaryExpression*>(syntax));
		break;

#define COROGLL_X(n) case SyntaxKind::n ## Expression:
	COROGLL_INVOKE_OPERATOR(COROGLL_X);
#undef COROGLL_X
		PrintContent(os, nextIndent, static_cast<InvokeExpression*>(syntax));
		break;

#define COROGLL_X(n) case SyntaxKind::n ## AccessExpression:
	COROGLL_ACCESS_OPERATOR(COROGLL_X);
#undef COROGLL_X
		PrintContent(os, nextIndent, static_cast<AccessExpression*>(syntax));
		break;
	}
//	PrintIndent(os, indent);
//	os << ")\n";
}

} // namespace

void Print(std::ostream& os, Syntax* syntax)
{
	PrintSyntax(os, 0, syntax);
}
