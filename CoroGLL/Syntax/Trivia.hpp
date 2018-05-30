#pragma once

#include "Lexeme.hpp"
#include "SourcePos.hpp"
#include "Syntax.hpp"
#include "../Core/Types.hpp"

#include <string_view>

namespace CoroGLL::Ast
{
	struct Trivia : Syntax, Lexeme
	{
		typedef Trivia CommonType;

	protected:
		Trivia(SourcePos sourcePos, SyntaxKind syntaxKind)
			: Syntax(syntaxKind), Lexeme(sourcePos)
		{
		}
	};

	struct BlockCommentTrivia : Trivia
	{
		BlockCommentTrivia(SourcePos pos, std::string_view content)
			: Trivia(pos, SyntaxKind::BlockCommentTrivia)
			, content(content)
		{
		}

	private:
		std::string_view content;
	};

	struct LineCommentTrivia : Trivia
	{
		LineCommentTrivia(SourcePos pos, std::string_view content, std::string_view newline)
			: Trivia(pos, SyntaxKind::LineCommentTrivia)
			, content(content)
			, newline(newline)
		{
		}

		std::string_view NewLine() const
		{
			return newline;
		}

		bool HasNewLine() const
		{
			return !newline.empty();
		}

	private:
		std::string_view content;
		std::string_view newline;
	};

	struct WhiteSpaceTrivia : Trivia
	{
		WhiteSpaceTrivia(SourcePos pos, std::string_view content, std::string_view newline)
			: Trivia(pos, SyntaxKind::WhiteSpaceTrivia)
			, content(content)
			, newline(newline)
		{
		}

		std::string_view NewLine() const
		{
			return newline;
		}

		bool HasNewLine() const
		{
			return !newline.empty();
		}

	private:
		std::string_view content;
		std::string_view newline;
	};

	struct ErrorCharTrivia : Trivia
	{
		ErrorCharTrivia(SourcePos sourcePos, std::string_view content)
			: Trivia(sourcePos, SyntaxKind::ErrorCharTrivia)
			, content(content)
		{
		}

	private:
		std::string_view content;
	};

	inline bool IsTrivia(SyntaxKind syntaxKind)
	{
		switch (syntaxKind)
		{
#define COROGLL_X(n) case SyntaxKind::n ## Trivia:
		COROGLL_TRIVIA(COROGLL_X)
#undef COROGLL_X
			return true;
		}
		return false;
	}

	inline bool IsTrivia(const Syntax* syntax)
	{
		return IsTrivia(syntax->Kind());
	}
}
