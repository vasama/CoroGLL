#include "Parser.hpp"

#include "Core/Types.hpp"
#include "Lexer.hpp"
#include "ParserCore.hpp"
#include "Syntax/Expression.hpp"

using namespace CoroGLL;
using namespace CoroGLL::Ast;

namespace {

using CoroGLL::Private::ParserCore::Result;
using CoroGLL::Private::ParserCore::ParseContext;

typedef ParseContext Ctx;

enum class Flags
{
	None = 0,

	Root     = 1 << 0,
	Function = 1 << 1,
	Struct   = 1 << 2,

	Switch   = 1 << 8,
	Loop     = 1 << 9,

	TypeExpr  = 1 << 16,
	ConstExpr = 1 << 17,
};

constexpr Flags operator&(Flags a, Flags b)
{
	typedef std::underlying_type_t<Flags> T;
	return (Flags)(((T)a) & ((T)b));
}

constexpr Flags operator|(Flags a, Flags b)
{
	typedef std::underlying_type_t<Flags> T;
	return (Flags)(((T)a) | ((T)b));
}

constexpr Flags operator~(Flags a)
{
	typedef std::underlying_type_t<Flags> T;
	return (Flags)(~((T)a));
}

enum class Precedence : i32
{
	Expression = 0,

	Assignment,
	Ternary,
	Coalescing,
	LogicalOr,
	LogicalAnd,
	Equality,
	Relation,
	Or,
	Xor,
	And,
	Shift,
	Additive,
	Multiplicative,
	UnaryPrefix, //TODO: separate definite unary prefix and potential unary prefix?
	TypeCast,
	UnaryPostfix,
	Invoke,
	Access,
	Primary,
};

bool IsWordToken(SyntaxKind syntaxKind)
{
	switch (syntaxKind)
	{
	COROGLL_CASE_SYNTAXKIND_KEYWORD
	case SyntaxKind::NameToken:
		return true;
	}
	return false;
}

bool IsTypeExpression(Expression* expression)
{
	switch (expression->Kind())
	{
	case SyntaxKind::MetaExpression:
	case SyntaxKind::WordExpression:
	case SyntaxKind::ScopeAccessExpression:
	case SyntaxKind::SpecializationExpression:
		return true;
	}
	return false;
}

namespace Rules {

Result<Expression> ParseExpression(Ctx* ctx, Flags flags, Precedence precedence);

Result<Argument> ParseArgument(Ctx* ctx, Flags flags)
{
	WordToken* nameToken = nullptr;
	Token* colonToken = nullptr;

	Expression* expression;

	switch (ctx->PeekToken()->Kind())
	{
	COROGLL_CASE_SYNTAXKIND_KEYWORD
	case SyntaxKind::NameToken:
		if (ctx->PeekToken(1)->Kind() == SyntaxKind::ColonSymbol)
		{
			nameToken = static_cast<WordToken*>(ctx->EatToken());
			colonToken = ctx->EatToken();
		}

	default:
		expression = co_await ctx->Parse(ParseExpression, flags, Precedence::Expression);
	}

	return ctx->CreateSyntax<Argument>(nameToken, colonToken, expression);
}

Result<ArgumentList> ParseArgumentList(Ctx* ctx, Flags flags)
{
	std::vector<Argument*> arguments;

	switch (ctx->PeekToken()->Kind())
	{
	case SyntaxKind::RParenSymbol:
	case SyntaxKind::RBrackSymbol:
	case SyntaxKind::RAngleSymbol:
		goto exit;
	}

	arguments.push_back(co_await ctx->Parse(ParseArgument, flags));
	while (ctx->PeekToken()->Kind() == SyntaxKind::CommaSymbol)
	{
		ctx->EatToken(); //TODO: keep the commas
		arguments.push_back(co_await ctx->Parse(ParseArgument, flags));
	}

exit:
	return ctx->CreateSyntax<ArgumentList>(ctx->CreateSyntaxList<Argument>(arguments.begin(), arguments.end()));
}

Result<Expression> ParseParensExpression(Ctx* ctx, Flags flags)
{
	Token* openToken = ctx->EatToken();
	Assert(openToken->Kind() == SyntaxKind::LParenSymbol);

	Expression* expression = co_await ctx->Parse(ParseExpression, flags, Precedence::Expression);

	Token* closeToken;
	if (ctx->PeekToken()->Kind() != SyntaxKind::RParenSymbol)
	{
		co_await ctx->SetError();
		Assert(false);
	}
	else closeToken = ctx->EatToken();

	co_return ctx->CreateSyntax<ParenthesizedExpression>(openToken, expression, closeToken);
}

Result<Expression> ParseCastExpression(Ctx* ctx, Flags flags)
{
	Token* openToken = ctx->EatToken();
	Assert(openToken->Kind() == SyntaxKind::LParenSymbol);

	Expression* typeExpression = co_await ctx->Parse(ParseExpression, flags | Flags::TypeExpr, Precedence::Expression);

	Token* closeToken;
	if (ctx->PeekToken()->Kind() != SyntaxKind::RParenSymbol)
	{
		co_await ctx->SetError();
		Assert(false);
	}
	else closeToken = ctx->EatToken();

	Expression* expression = co_await ctx->Parse(ParseExpression, flags, Precedence::TypeCast);
	co_return ctx->CreateSyntax<CastExpression>(openToken, typeExpression, closeToken, expression);
}

Result<Expression> ParseLambdaExpression(Ctx* ctx, Flags flags)
{
	co_await ctx->SetError();
	Assert(false);

	co_return nullptr;
}

Result<Expression> ParseMetaExpression(Ctx* ctx, Flags flags)
{
	Token* dollarToken = ctx->EatToken();
	Assert(dollarToken->Kind() == SyntaxKind::DollarSymbol);

	Token* openToken = ctx->EatToken();
	Assert(openToken->Kind() == SyntaxKind::LParenSymbol);

	Expression* expression = co_await ctx->Parse(ParseExpression, flags & ~Flags::TypeExpr, Precedence::Expression);

	Token* closeToken;
	if (ctx->PeekToken()->Kind() != SyntaxKind::RParenSymbol)
	{
		co_await ctx->SetError();
		Assert(false);
	}
	else closeToken = ctx->EatToken();

	co_return ctx->CreateSyntax<MetaExpression>(dollarToken, openToken, expression, closeToken);
}

Result<Expression> ParseCallExpression(Ctx* ctx, Flags flags, Expression* expression)
{
	Token* openToken = ctx->EatToken();
	Assert(openToken->Kind() == SyntaxKind::LParenSymbol);

	ArgumentList* arguments = co_await ctx->Parse(ParseArgumentList, flags);

	Token* closeToken;
	if (ctx->PeekToken()->Kind() != SyntaxKind::RParenSymbol)
	{
		co_await ctx->SetError();
		Assert(false);
	}
	else closeToken = ctx->EatToken();

	co_return ctx->CreateSyntax<InvokeExpression>(InvokeOperator::Call, expression, openToken, arguments, closeToken);
}

Result<Expression> ParseIndexExpression(Ctx* ctx, Flags flags, Expression* expression)
{
	Token* openToken = ctx->EatToken();
	Assert(openToken->Kind() == SyntaxKind::LBrackSymbol);

	ArgumentList* arguments = co_await ctx->Parse(ParseArgumentList, flags);

	Token* closeToken;
	if (ctx->PeekToken()->Kind() != SyntaxKind::RBrackSymbol)
	{
		co_await ctx->SetError();
		Assert(false);
	}
	else closeToken = ctx->EatToken();

	co_return ctx->CreateSyntax<InvokeExpression>(InvokeOperator::Index, expression, openToken, arguments, closeToken);
}

Result<Expression> ParseSpecializationExpression(Ctx* ctx, Flags flags, Expression* expression)
{
	Token* openToken = ctx->EatToken();
	Assert(openToken->Kind() == SyntaxKind::LAngleSymbol);

	ArgumentList* arguments = co_await ctx->Parse(ParseArgumentList, flags | Flags::TypeExpr);

	Token* closeToken;
	if (ctx->PeekToken()->Kind() != SyntaxKind::RAngleSymbol)
	{
		co_await ctx->SetError();
		Assert(false);
	}
	else closeToken = ctx->EatToken();

/*	switch (ctx->PeekToken()->Kind())
	{
	COROGLL_CASE_SYNTAXKIND_KEYWORD
	case SyntaxKind::NameToken:
	case SyntaxKind::LParenSymbol:
	case SyntaxKind::CharLiteralToken:
	case SyntaxKind::StringLiteralToken:
	case SyntaxKind::NumericLiteralToken:
	case SyntaxKind::DollarSymbol:
		co_await ctx->SetError();
		Assert(false);
	} */

	co_return ctx->CreateSyntax<InvokeExpression>(InvokeOperator::Specialization, expression, openToken, arguments, closeToken);
}

Result<Expression> ParseScopeAccessExpression(Ctx* ctx, Flags flags, Expression* expression)
{
	Token* operatorToken = ctx->EatToken();
	Assert(operatorToken->Kind() == SyntaxKind::ScopeSymbol);

	WordToken* nameToken;
	switch (ctx->PeekToken()->Kind())
	{
	COROGLL_CASE_SYNTAXKIND_KEYWORD
	case SyntaxKind::NameToken:
		nameToken = static_cast<WordToken*>(ctx->EatToken());
		break;

	default:
		co_await ctx->SetError();
		Assert(false);
	}

	co_return ctx->CreateSyntax<AccessExpression>(AccessOperator::Scope, expression, operatorToken, nameToken);
}

Result<Expression> ParseDirectAccessExpression(Ctx* ctx, Flags flags, Expression* expression)
{
	Token* operatorToken = ctx->EatToken();
	Assert(operatorToken->Kind() == SyntaxKind::DotSymbol);

	WordToken* nameToken;
	switch (ctx->PeekToken()->Kind())
	{
	COROGLL_CASE_SYNTAXKIND_KEYWORD
	case SyntaxKind::NameToken:
		nameToken = static_cast<WordToken*>(ctx->EatToken());
		break;

	default:
		co_await ctx->SetError();
		Assert(false);
	}

	co_return ctx->CreateSyntax<AccessExpression>(AccessOperator::Direct, expression, operatorToken, nameToken);
}

Result<Expression> ParseIndirectAccessExpression(Ctx* ctx, Flags flags, Expression* expression)
{

	Token* operatorToken = ctx->EatToken();
	Assert(operatorToken->Kind() == SyntaxKind::ArrowSymbol);

	WordToken* nameToken;
	switch (ctx->PeekToken()->Kind())
	{
	COROGLL_CASE_SYNTAXKIND_KEYWORD
	case SyntaxKind::NameToken:
		nameToken = static_cast<WordToken*>(ctx->EatToken());
		break;

	default:
		co_await ctx->SetError();
		Assert(false);
	}

	co_return ctx->CreateSyntax<AccessExpression>(AccessOperator::Indirect, expression, operatorToken, nameToken);
}

Result<Expression> ParsePrimaryExpression(Ctx* ctx, Flags flags)
{
	Expression* expression = nullptr; //TODO: no init

	switch (ctx->PeekToken()->Kind())
	{
	case SyntaxKind::ScopeSymbol:
		{
			if (!IsWordToken(ctx->PeekToken(1)->Kind()))
			{
				co_await ctx->SetError();
				Assert(false);
			}

			//TODO: rooted name
			co_await ctx->SetError();
			Assert(false);
		}
		break;

	COROGLL_CASE_SYNTAXKIND_KEYWORD
	case SyntaxKind::NameToken:
		expression = ctx->CreateSyntax<WordExpression>(static_cast<WordToken*>(ctx->EatToken()));
		break;

	case SyntaxKind::CharLiteralToken:
	case SyntaxKind::StringLiteralToken:
	case SyntaxKind::NumericLiteralToken:
		expression = ctx->CreateSyntax<LiteralExpression>(static_cast<LiteralToken*>(ctx->EatToken()));
		break;

	case SyntaxKind::DollarSymbol:
		if (ctx->PeekToken(1)->Kind() == SyntaxKind::LParenSymbol)
		{
			expression = co_await ctx->Parse(ParseMetaExpression, flags);
		}
		else
		{
			co_await ctx->SetError();
			Assert(false);
		}
		break;

	case SyntaxKind::LParenSymbol:
		if ((flags & Flags::TypeExpr) != Flags::None)
		{
			co_await ctx->SetError();
			Assert(false);
		}

		switch (co_await ctx->Fork(2))
		{
		case 0: // parens expression
			expression = co_await ctx->Parse(ParseParensExpression, flags);
			break;

		case 1: // cast expression
			expression = co_await ctx->Parse(ParseCastExpression, flags);
			break;

	/*	case 2: // lambda expression
			expression = co_await ctx->Parse(ParseLambdaExpression, flags);
			break; */
		}
		break;
	}

	while (true)
	{
		switch (ctx->PeekToken()->Kind())
		{
		case SyntaxKind::LParenSymbol:
			if ((flags & Flags::TypeExpr) != Flags::None)
			{
				co_await ctx->SetError();
				Assert(false);
			}

			expression = co_await ctx->Parse(ParseCallExpression, flags, expression);
			break;

		case SyntaxKind::LBrackSymbol:
			if ((flags & Flags::TypeExpr) != Flags::None)
			{
				co_await ctx->SetError();
				Assert(false);
			}

			expression = co_await ctx->Parse(ParseIndexExpression, flags, expression);
			break;

		case SyntaxKind::LAngleSymbol:
			if ((flags & Flags::TypeExpr) != Flags::None)
				goto parseSpecializationExpression;

			if (!IsTypeExpression(expression))
				goto parseLessThanExpression;

			switch (co_await ctx->Fork(2))
			{
			case 0: goto parseSpecializationExpression;
			case 1: goto parseLessThanExpression;
			}

		parseSpecializationExpression:
			expression = co_await ctx->Parse(ParseSpecializationExpression, flags, expression);

			switch (ctx->PeekToken()->Kind())
			{
			COROGLL_CASE_SYNTAXKIND_KEYWORD
			case SyntaxKind::NameToken:
			case SyntaxKind::LParenSymbol:
			case SyntaxKind::CharLiteralToken:
			case SyntaxKind::StringLiteralToken:
			case SyntaxKind::NumericLiteralToken:
			case SyntaxKind::DollarSymbol:
				co_await ctx->SetError();
				Assert(false);
			}
			break;

		parseLessThanExpression:
			goto exit;

		case SyntaxKind::ScopeSymbol:
			expression = co_await ctx->Parse(ParseScopeAccessExpression, flags, expression);
			break;

		case SyntaxKind::DotSymbol:
			if ((flags & Flags::TypeExpr) != Flags::None)
			{
				co_await ctx->SetError();
				Assert(false);
			}

			expression = co_await ctx->Parse(ParseDirectAccessExpression, flags, expression);
			break;

		case SyntaxKind::ArrowSymbol:
			if ((flags & Flags::TypeExpr) != Flags::None)
			{
				co_await ctx->SetError();
				Assert(false);
			}

			expression = co_await ctx->Parse(ParseIndirectAccessExpression, flags, expression);
			break;

		case SyntaxKind::IncrementSymbol:
			if ((flags & Flags::TypeExpr) != Flags::None)
			{
				co_await ctx->SetError();
				Assert(false);
			}

			expression = ctx->CreateSyntax<UnaryExpression>(UnaryOperator::PostfixIncrement, ctx->EatToken(), expression);
			break;

		case SyntaxKind::DecrementSymbol:
			if ((flags & Flags::TypeExpr) != Flags::None)
			{
				co_await ctx->SetError();
				Assert(false);
			}

			expression = ctx->CreateSyntax<UnaryExpression>(UnaryOperator::PostfixDecrement, ctx->EatToken(), expression);
			break;

		default:
			goto exit;
		}
	}
exit:

	co_return expression;
}

Result<Expression> ParseUnaryExpression(Ctx* ctx, Flags flags)
{
	switch (ctx->PeekToken()->Kind())
	{
		UnaryOperator unaryOperator;

	case SyntaxKind::AddSymbol:
		unaryOperator = UnaryOperator::Plus;
		goto parseUnaryPrefixOperator;

	case SyntaxKind::AndSymbol:
		unaryOperator = UnaryOperator::Addressof;
		goto parseUnaryPrefixOperator;

	case SyntaxKind::AwaitKeyword:
		unaryOperator = UnaryOperator::Await;
		goto parseUnaryPrefixOperator;

	case SyntaxKind::DecrementSymbol:
		unaryOperator = UnaryOperator::PrefixDecrement;
		goto parseUnaryPrefixOperator;

	case SyntaxKind::MulSymbol:
		unaryOperator = UnaryOperator::Indirection;
		goto parseUnaryPrefixOperator;

	case SyntaxKind::NotSymbol:
		unaryOperator = UnaryOperator::Not;
		goto parseUnaryPrefixOperator;

	case SyntaxKind::IncrementSymbol:
		unaryOperator = UnaryOperator::PrefixIncrement;
		goto parseUnaryPrefixOperator;

	case SyntaxKind::LogicalNotSymbol:
		unaryOperator = UnaryOperator::LogicalNot;
		goto parseUnaryPrefixOperator;

	case SyntaxKind::SubSymbol:
		unaryOperator = UnaryOperator::Minus;
		goto parseUnaryPrefixOperator;

	parseUnaryPrefixOperator:
		Token* operatorToken = ctx->EatToken();
		Expression* operand = co_await ctx->Parse(ParseExpression, flags, Precedence::UnaryPrefix);
		co_return ctx->CreateSyntax<UnaryExpression>(unaryOperator, operatorToken, operand);
	}

	co_return co_await ctx->Parse(ParsePrimaryExpression, flags);
}

Result<Expression> ParseExpression(Ctx* ctx, Flags flags, Precedence precedence)
{
	Expression* leftOperand = co_await ctx->Parse(ParseUnaryExpression, flags);

	if ((flags & Flags::TypeExpr) != Flags::None)
		co_return leftOperand;

	while (true)
	{
		BinaryOperator binaryOperator;
		Precedence operatorPrecedence;
		bool leftAssociative = true;

		Token* operatorToken = ctx->PeekToken();
		switch (operatorToken->Kind())
		{
		case SyntaxKind::AddSymbol:
			binaryOperator = BinaryOperator::Addition;
			operatorPrecedence = Precedence::Additive;
			break;

		case SyntaxKind::AndSymbol:
			binaryOperator = BinaryOperator::And;
			operatorPrecedence = Precedence::And;
			break;

		case SyntaxKind::AssignAddSymbol:
			binaryOperator = BinaryOperator::AdditionAssignment;
			operatorPrecedence = Precedence::Assignment;
			leftAssociative = false;
			break;

		case SyntaxKind::AssignAndSymbol:
			binaryOperator = BinaryOperator::AndAssignment;
			operatorPrecedence = Precedence::Assignment;
			leftAssociative = false;
			break;

		case SyntaxKind::AssignDivSymbol:
			binaryOperator = BinaryOperator::DivisionAssignment;
			operatorPrecedence = Precedence::Assignment;
			leftAssociative = false;
			break;

		case SyntaxKind::AssignLhsSymbol:
			binaryOperator = BinaryOperator::LeftShiftAssignment;
			operatorPrecedence = Precedence::Assignment;
			leftAssociative = false;
			break;

		case SyntaxKind::AssignModSymbol:
			binaryOperator = BinaryOperator::ModuloAssignment;
			operatorPrecedence = Precedence::Assignment;
			leftAssociative = false;
			break;

		case SyntaxKind::AssignMulSymbol:
			binaryOperator = BinaryOperator::MultiplicationAssignment;
			operatorPrecedence = Precedence::Assignment;
			leftAssociative = false;
			break;

		case SyntaxKind::AssignNotSymbol:
			binaryOperator = BinaryOperator::NotAssignment;
			operatorPrecedence = Precedence::Assignment;
			leftAssociative = false;
			break;

		case SyntaxKind::AssignOrSymbol:
			binaryOperator = BinaryOperator::OrAssignment;
			operatorPrecedence = Precedence::Assignment;
			leftAssociative = false;
			break;

		case SyntaxKind::AssignSubSymbol:
			binaryOperator = BinaryOperator::SubtractionAssignment;
			operatorPrecedence = Precedence::Assignment;
			leftAssociative = false;
			break;

		case SyntaxKind::AssignSymbol:
			binaryOperator = BinaryOperator::Assignment;
			operatorPrecedence = Precedence::Assignment;
			leftAssociative = false;
			break;

		case SyntaxKind::AssignXorSymbol:
			binaryOperator = BinaryOperator::XorAssignment;
			operatorPrecedence = Precedence::Assignment;
			leftAssociative = false;
			break;

		case SyntaxKind::DivSymbol:
			binaryOperator = BinaryOperator::Division;
			operatorPrecedence = Precedence::Multiplicative;
			break;

		case SyntaxKind::EqualSymbol:
			binaryOperator = BinaryOperator::Equal;
			operatorPrecedence = Precedence::Equality;
			break;

		case SyntaxKind::GreaterSymbol:
			if (operatorToken->TrailingTrivia().Size() == 0)
			{
				Token* operatorToken2 = ctx->PeekToken(1);
				if (operatorToken2->LeadingTrivia().Size() != 0)
					goto greaterThan;

				switch (operatorToken2->Kind())
				{
				case SyntaxKind::GreaterSymbol:
					if (operatorToken2->TrailingTrivia().Size() == 0)
					{
						Token* operatorToken3 = ctx->PeekToken(2);
						if (operatorToken3->Kind() == SyntaxKind::AssignSymbol && operatorToken3->LeadingTrivia().Size() == 0)
						{
							binaryOperator = BinaryOperator::RightShiftAssignment;
							operatorPrecedence = Precedence::Assignment;
							leftAssociative = false;
							break;
						}
					}
					binaryOperator = BinaryOperator::RightShift;
					operatorPrecedence = Precedence::Shift;
					break;

				case SyntaxKind::AssignSymbol:
					binaryOperator = BinaryOperator::GreaterThanOrEqual;
					operatorPrecedence = Precedence::Relation;
					break;

				default:
					goto greaterThan;
				}
			}
			else
			{
			greaterThan:
				binaryOperator = BinaryOperator::GreaterThan;
				operatorPrecedence = Precedence::Relation;
			}
			break;

		case SyntaxKind::ModSymbol:
			binaryOperator = BinaryOperator::Modulo;
			operatorPrecedence = Precedence::Multiplicative;
			break;

		case SyntaxKind::MulSymbol:
			binaryOperator = BinaryOperator::Multiplication;
			operatorPrecedence = Precedence::Multiplicative;
			break;

		case SyntaxKind::NotEqualSymbol:
			binaryOperator = BinaryOperator::NotEqual;
			operatorPrecedence = Precedence::Equality;
			break;

		case SyntaxKind::CoalescingSymbol:
			binaryOperator = BinaryOperator::Coalescing;
			operatorPrecedence = Precedence::Coalescing;
			leftAssociative = false;
			break;

		case SyntaxKind::LeftShiftSymbol:
			binaryOperator = BinaryOperator::LeftShift;
			operatorPrecedence = Precedence::Shift;
			break;

		case SyntaxKind::LessSymbol:
			binaryOperator = BinaryOperator::LessThan;
			operatorPrecedence = Precedence::Relation;
			break;

		case SyntaxKind::LessOrEqualSymbol:
			binaryOperator = BinaryOperator::LessThanOrEqual;
			operatorPrecedence = Precedence::Relation;
			break;

		case SyntaxKind::LogicalAndSymbol:
			binaryOperator = BinaryOperator::LogicalAnd;
			operatorPrecedence = Precedence::LogicalAnd;
			break;

		case SyntaxKind::LogicalOrSymbol:
			binaryOperator = BinaryOperator::LogicalOr;
			operatorPrecedence = Precedence::LogicalOr;
			break;

		case SyntaxKind::OrSymbol:
			binaryOperator = BinaryOperator::Or;
			operatorPrecedence = Precedence::Or;
			break;

		case SyntaxKind::SubSymbol:
			binaryOperator = BinaryOperator::Subtraction;
			operatorPrecedence = Precedence::Additive;
			break;

		case SyntaxKind::XorSymbol:
			binaryOperator = BinaryOperator::Xor;
			operatorPrecedence = Precedence::Xor;
			break;

		default:
			goto exit;
		}

		if (operatorPrecedence < precedence)
			break;

		if (operatorPrecedence == precedence && leftAssociative)
			break;

		operatorToken = ctx->EatToken();

		//TODO: combine tokens
		switch (binaryOperator)
		{
		case BinaryOperator::RightShiftAssignment:
			ctx->EatToken();
			ctx->EatToken();
			break;

		case BinaryOperator::GreaterThanOrEqual:
			ctx->EatToken();
			break;

		case BinaryOperator::RightShift:
			ctx->EatToken();
			break;
		}

		Expression* rightOperand = co_await ctx->Parse(ParseExpression, flags, operatorPrecedence);
		leftOperand = ctx->CreateSyntax<BinaryExpression>(binaryOperator, leftOperand, operatorToken, rightOperand);
	}
exit:

	if (precedence <= Precedence::Ternary && ctx->PeekToken()->Kind() == SyntaxKind::QuestionSymbol)
	{
		Token* questionToken = ctx->EatToken();
		Expression* trueExpression;

		Token* colonToken;
		Expression* falseExpression;

		if (ctx->PeekToken()->Kind() == SyntaxKind::ColonSymbol)
		{
			colonToken = ctx->EatToken();
			trueExpression = nullptr;
		}
		else
		{
			trueExpression = co_await ctx->Parse(ParseExpression, flags, Precedence::Expression);

			if (ctx->PeekToken()->Kind() == SyntaxKind::ColonSymbol)
			{
				colonToken = ctx->EatToken();
			}
			else
			{
				co_await ctx->SetError();
				Assert(false);
			}
		}

		falseExpression = co_await ctx->Parse(ParseExpression, flags, Precedence::Expression);
		leftOperand = ctx->CreateSyntax<TernaryExpression>(leftOperand, questionToken, trueExpression, colonToken, falseExpression);
	}

	co_return leftOperand;
}

} // namespace Rules

template<typename TSyntax, typename... TParams>
Result<TSyntax> ParseRoot(Ctx* ctx, Result<TSyntax>(*func)(Ctx*, TParams...), TParams... args)
{
	TSyntax* syntax = co_await ctx->Parse(func, std::move(args)...);

	if (ctx->PeekToken()->Kind() != SyntaxKind::EofToken)
	{
		co_await ctx->SetError();
		Assert(false);
	}

	co_return syntax;
}

template<typename TSyntax, typename... TParams, typename... TArgs>
SyntaxTree ParseInternal(std::string_view text, Result<TSyntax>(*func)(Ctx*, TParams...), TArgs&&... args)
{
	Private::SyntaxTreeContext treeContext;
	std::vector<Token*> tokenVector = Private::Lex(text, &treeContext);

	Span<Token*> tokens(tokenVector.data(), tokenVector.size());
	TSyntax* syntax = CoroGLL::Private::ParserCore::Parse(tokens, &treeContext,
		ParseRoot<TSyntax, TParams...>, func, std::forward<TArgs>(args)...);

	return Private::SyntaxTreeAttorney::CreateSyntaxTree(syntax, std::move(treeContext));
}

} // namespace

SyntaxTree CoroGLL::ParseExpression(std::string_view text)
{
	return ParseInternal(text, Rules::ParseExpression, Flags::None, Precedence::Expression);
}
