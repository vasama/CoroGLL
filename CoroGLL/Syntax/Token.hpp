#pragma once

#include "Lexeme.hpp"
#include "SourcePos.hpp"
#include "Syntax.hpp"
#include "Trivia.hpp"
#include "../Core/Rational.hpp"
#include "../Core/Span.hpp"
#include "../Core/Types.hpp"

#include <string_view>

namespace CoroGLL::Ast {

struct TokenInfo
{
	SourcePos SourcePosition;
	Span<Trivia*> LeadingTrivia;
	Span<Trivia*> TrailingTrivia;
};

enum class Symbol : u32
{
#define COROGLL_X(n) n = (u32)SyntaxKind::n ## Symbol,
	COROGLL_SYMBOL(COROGLL_X)
#undef COROGLL_X
};

enum class Keyword : u32
{
#define COROGLL_X(n, s) n = (u32)SyntaxKind::n ## Keyword,
	COROGLL_KEYWORD(COROGLL_X)
#undef COROGLL_X
};

std::string_view ToString(Keyword);

bool ResolveKeyword(std::string_view, Keyword& out);

struct Token : Syntax, Lexeme
{
	typedef Token CommonType;

	Span<Trivia*> LeadingTrivia() const
	{
		return leadingTrivia;
	}

	Span<Trivia*> TrailingTrivia() const
	{
		return trailingTrivia;
	}

protected:
	Token(const TokenInfo& tokenInfo, SyntaxKind syntaxKind)
		: Syntax(syntaxKind)
		, Lexeme(tokenInfo.SourcePosition)
		, leadingTrivia(tokenInfo.LeadingTrivia)
		, trailingTrivia(tokenInfo.TrailingTrivia)
	{
	}

private:
	Span<Trivia*> leadingTrivia;
	Span<Trivia*> trailingTrivia;
};

struct SymbolToken : Token
{
	SymbolToken(const TokenInfo& tokenInfo, Symbol symbol)
		: Token(tokenInfo, (SyntaxKind)symbol)
	{
	}
};

struct WordToken : Token
{
	std::string_view String() const;

protected:
	WordToken(const TokenInfo& tokenInfo, SyntaxKind syntaxKind)
		: Token(tokenInfo, syntaxKind)
	{
	}
};

struct NameToken : WordToken
{
	NameToken(const TokenInfo& tokenInfo, std::string_view content, bool verbatim)
		: WordToken(tokenInfo, SyntaxKind::NameToken)
		, content(content)
		, verbatim(verbatim)
	{
	}

	std::string_view String() const
	{
		return content;
	}

	bool Verbatim() const
	{
		return verbatim;
	}

private:
	std::string_view content;
	bool verbatim;
};

struct KeywordToken : WordToken
{
	KeywordToken(const TokenInfo& tokenInfo, Keyword keyword)
		: WordToken(tokenInfo, (SyntaxKind)keyword)
	{
	}

	std::string_view String() const
	{
		return ToString((Keyword)Syntax::Kind());
	}
};

struct LiteralToken : Token
{
	WordToken* Type() const
	{
		return type;
	}

protected:
	LiteralToken(const TokenInfo& tokenInfo, SyntaxKind syntaxKind, WordToken* type)
		: Token(tokenInfo, syntaxKind)
		, type(type)
	{
	}

private:
	WordToken* type;
};

struct CharLiteralToken : LiteralToken
{
	CharLiteralToken(const TokenInfo& tokenInfo, std::string_view content, WordToken* type)
		: LiteralToken(tokenInfo, SyntaxKind::CharLiteralToken, type)
		, content(content)
	{
	}

	std::string_view Value() const
	{
		return content;
	}

private:
	std::string_view content;
};

struct StringLiteralToken : LiteralToken
{
	StringLiteralToken(const TokenInfo& tokenInfo, std::string_view content, WordToken* type)
		: LiteralToken(tokenInfo, SyntaxKind::StringLiteralToken, type)
		, content(content)
	{
	}

	std::string_view Value() const
	{
		return content;
	}

private:
	std::string_view content;
};

struct NumericLiteralToken : LiteralToken
{
	NumericLiteralToken(const TokenInfo& tokenInfo, Rational content, WordToken* type)
		: LiteralToken(tokenInfo, SyntaxKind::NumericLiteralToken, type)
		, content(content)
	{
	}

	Rational Value() const
	{
		return content;
	}

private:
	Rational content;
};

struct MissingToken : Token
{
	MissingToken(SyntaxKind expectedSyntaxKind, Token* actualToken)
		: MissingToken(TokenInfo{ actualToken->Pos(), Span<Trivia*>(), Span<Trivia*>() }, expectedSyntaxKind)
	{
	}

private:
	MissingToken(const TokenInfo& tokenInfo, SyntaxKind expectedSyntaxKind)
		: Token(tokenInfo, SyntaxKind::MissingToken), expectedSyntaxKind(expectedSyntaxKind)
	{
	}

	SyntaxKind expectedSyntaxKind;
};

struct EofToken : Token
{
	EofToken(const TokenInfo& tokenInfo)
		: Token(tokenInfo, SyntaxKind::EofToken)
	{
	}
};

inline bool IsToken(SyntaxKind syntaxKind)
{
	switch (syntaxKind)
	{
#define COROGLL_X(n) case SyntaxKind::n ## Token:
	COROGLL_TOKEN(COROGLL_X)
#undef COROGLL_X
#define COROGLL_X(n) case SyntaxKind::n ## Symbol:
	COROGLL_SYMBOL(COROGLL_X)
#undef COROGLL_X
#define COROGLL_X(n, s) case SyntaxKind::n ## Keyword:
	COROGLL_KEYWORD(COROGLL_X)
#undef COROGLL_X
		return true;
	}
	return false;
}

inline bool IsToken(const Syntax* syntax)
	{
		return IsToken(syntax->Kind());
	}

} // namespace CoroGLL::Ast

inline std::string_view CoroGLL::Ast::WordToken::String() const
{
	if (Syntax::Kind() == SyntaxKind::NameToken)
		return static_cast<const NameToken*>(this)->String();
	return static_cast<const KeywordToken*>(this)->String();
}
