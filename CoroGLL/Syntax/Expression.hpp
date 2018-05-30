#pragma once

#include "Syntax.hpp"
#include "Token.hpp"
#include "../Core/Types.hpp"
#include "../Core/Span.hpp"

namespace CoroGLL::Ast
{
	struct ParameterDeclaration;
	struct FunctionBody;

	enum class UnaryOperator : u32
	{
#define COROGLL_X(n) n = (u32)SyntaxKind::n ## Expression,
		COROGLL_UNARY_OPERATOR(COROGLL_X)
#undef COROGLL_X
	};

	enum class BinaryOperator : u32
	{
#define COROGLL_X(n) n = (u32)SyntaxKind::n ## Expression,
		COROGLL_BINARY_OPERATOR(COROGLL_X)
#undef COROGLL_X
	};

	enum class InvokeOperator : u32
	{
#define COROGLL_X(n) n = (u32)SyntaxKind::n ## Expression,
		COROGLL_INVOKE_OPERATOR(COROGLL_X)
#undef COROGLL_X
	};

	enum class AccessOperator : u32
	{
#define COROGLL_X(n) n = (u32)SyntaxKind::n ## AccessExpression,
		COROGLL_ACCESS_OPERATOR(COROGLL_X)
#undef COROGLL_X
	};

	struct Expression : Syntax
	{
		typedef Expression CommonType;

	protected:
		Expression(SyntaxKind syntaxKind)
			: Syntax(syntaxKind)
		{
		}
	};
	
	struct CastExpression : Expression
	{
		CastExpression(Token* openToken
		             , Expression* type
		             , Token* closeToken
		             , Expression* expression
		)	: Expression(SyntaxKind::CastExpression)
			, type(type)
			, expression(expression)
			, openToken(openToken)
			, closeToken(closeToken)
		{
		}

		Expression* type;
		Expression* expression;
		Token* openToken;
		Token* closeToken;
	};

	struct LiteralExpression : Expression
	{
		LiteralExpression(LiteralToken* literalToken)
			: Expression(SyntaxKind::LiteralExpression)
			, literalToken(literalToken)
		{
		}

		LiteralToken* literalToken;
	};

	struct MetaExpression : Expression
	{
		MetaExpression(Token* dollarToken
		             , Token* openToken
		             , Expression* expression
		             , Token* closeToken
		)	: Expression(SyntaxKind::MetaExpression)
			, expression(expression)
			, dollarToken(dollarToken)
			, openToken(openToken)
			, closeToken(closeToken)
		{
		}

		Expression* expression;
		Token* dollarToken;
		Token* openToken;
		Token* closeToken;
	};
	
	struct ParenthesizedExpression : Expression
	{
		ParenthesizedExpression(Token* openToken
		                      , Expression* expression
		                      , Token* closeToken
		)	: Expression(SyntaxKind::ParenthesizedExpression)
			, expression(expression)
			, openToken(openToken)
			, closeToken(closeToken)
		{
		}

		Expression* expression;
		Token* openToken;
		Token* closeToken;
	};

	struct TernaryExpression : Expression
	{
		TernaryExpression(Expression* condition
		                , Token* questionToken
		                , Expression* trueExpression
		                , Token* colonToken
		                , Expression* falseExpression
		)	: Expression(SyntaxKind::TernaryExpression)
			, condition(condition)
			, trueExpression(trueExpression)
			, falseExpression(falseExpression)
			, questionToken(questionToken)
			, colonToken(colonToken)
		{
		}

		Expression* condition;
		Expression* trueExpression;
		Expression* falseExpression;
		Token* questionToken;
		Token* colonToken;
	};

	struct WordExpression : Expression
	{
		WordExpression(WordToken* nameToken)
			: Expression(SyntaxKind::WordExpression)
			, nameToken(nameToken)
		{
		}

		WordToken* nameToken;
	};

	struct WildcardExpression : Expression
	{
		WildcardExpression(Expression* expression
		                 , Token* operatorToken
		                 , Token* starToken
		)	: Expression(SyntaxKind::WildcardExpression)
			, expression(expression)
			, operatorToken(operatorToken)
			, starToken(starToken)
		{
		}

		Expression* expression;
		Token* operatorToken;
		Token* starToken;
	};

	struct UnaryExpression : Expression
	{
		UnaryExpression(UnaryOperator unaryOperator
		              , Token* operatorToken
		              , Expression* expression
		)	: Expression((SyntaxKind)unaryOperator)
			, expression(expression)
			, operatorToken(operatorToken)
		{
		}

		Expression* expression;
		Token* operatorToken;
	};

	struct BinaryExpression : Expression
	{
		BinaryExpression(BinaryOperator binaryOperator
		               , Expression* leftExpression
		               , Token* operatorToken
		               , Expression* rightExpression
		)	: Expression((SyntaxKind)binaryOperator)
			, leftExpression(leftExpression)
			, rightExpression(rightExpression)
			, operatorToken(operatorToken)
		{
		}

		Expression* leftExpression;
		Expression* rightExpression;
		Token* operatorToken;
	};

	struct Argument : Syntax
	{
		Argument(WordToken* nameToken
		       , Token* colonToken
		       , Expression* expression
		)	: Syntax(SyntaxKind::Argument)
			, nameToken(nameToken)
			, expression(expression)
			, colonToken(colonToken)
		{
		}

		WordToken* nameToken;
		Expression* expression;
		Token* colonToken;
	};

	struct ArgumentList : Syntax
	{
		ArgumentList(Span<Argument*> arguments)
			: Syntax(SyntaxKind::ArgumentList), arguments(arguments)
		{
		}

		Span<Argument*> arguments;
	};

	struct InvokeExpression : Expression
	{
		InvokeExpression(InvokeOperator invokeOperator
		               , Expression* expression
		               , Token* openToken
		               , ArgumentList* arguments
		               , Token* closeToken
		)	: Expression((SyntaxKind)invokeOperator)
			, expression(expression)
			, arguments(arguments)
			, openToken(openToken)
			, closeToken(closeToken)
		{
		}

		Expression* expression;
		ArgumentList* arguments;
		Token* openToken;
		Token* closeToken;
	};

	struct AccessExpression : Expression
	{
		AccessExpression(AccessOperator accessOperator
		               , Expression* expression
		               , Token* operatorToken
		               , WordToken* nameToken
		)	: Expression((SyntaxKind)accessOperator)
			, expression(expression)
			, nameToken(nameToken)
			, operatorToken(operatorToken)
		{
		}

		Expression* expression;
		WordToken* nameToken;
		Token* operatorToken;
	};

	struct ListExpression : Syntax
	{
		ListExpression(Expression* expression
		             , SymbolToken* commaToken
		)	: Syntax(SyntaxKind::ListExpression)
			, expression(expression)
			, commaToken(commaToken)
		{
		}

		Expression* expression;
		SymbolToken* commaToken;
	};

	inline bool IsExpression(SyntaxKind syntaxKind)
	{
		switch (syntaxKind)
		{
#define COROGLL_X(n) case SyntaxKind::n ## Expression:
		COROGLL_EXPRESSION(COROGLL_X)
		COROGLL_UNARY_OPERATOR(COROGLL_X)
		COROGLL_BINARY_OPERATOR(COROGLL_X)
		COROGLL_INVOKE_OPERATOR(COROGLL_X)
#undef COROGLL_X
#define COROGLL_X(n) case SyntaxKind::n ## AccessExpression :
		COROGLL_ACCESS_OPERATOR(COROGLL_X)
#undef COROGLL_X
			return true;
		}
		return false;
	}

	inline bool IsExpression(const Syntax* syntax)
	{
		return IsExpression(syntax->Kind());
	}
}
