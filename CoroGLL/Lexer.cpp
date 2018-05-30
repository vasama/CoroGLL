#include "Lexer.hpp"
#include "Core/Debug.hpp"
#include "Core/Span.hpp"
#include "Core/StringBuilder.hpp"
#include "Core/Types.hpp"
#include "Syntax/SourcePos.hpp"

#include <utility>
#include <vector>

#define WS_CHARS(X) \
	X(' ')  X('\t') X('\v') \
	X('\f') X('\r') X('\n') \

#define NUM_CHARS(X) \
	X('0') X('1') X('2') X('3') X('4') \
	X('5') X('6') X('7') X('8') X('9') \

#define WORD_CHARS(X) X('_') \
	X('a') X('A') X('b') X('B') \
	X('c') X('C') X('d') X('D') \
	X('e') X('E') X('f') X('F') \
	X('g') X('G') X('h') X('H') \
	X('i') X('I') X('j') X('J') \
	X('k') X('K') X('l') X('L') \
	X('m') X('M') X('n') X('N') \
	X('o') X('O') X('p') X('P') \
	X('q') X('Q') X('r') X('R') \
	X('s') X('S') X('t') X('T') \
	X('u') X('U') X('v') X('V') \
	X('w') X('W') X('y') X('Y') \
	X('x') X('X') X('z') X('Z') \

#define MAKE_CASE(x) case x:

#define CASE_WS_CHAR WS_CHARS(MAKE_CASE)
#define CASE_NUM_CHAR NUM_CHARS(MAKE_CASE)
#define CASE_WORD_CHAR WORD_CHARS(MAKE_CASE)

namespace {
	
using namespace CoroGLL;
using namespace CoroGLL::Ast;
using namespace CoroGLL::Private;

bool IsWhiteSpaceChar(char c)
{
	switch (c)
	{
	CASE_WS_CHAR
		return true;
	}
	return false;
}

bool IsNumChar(char c)
{
	switch (c)
	{
	CASE_NUM_CHAR
		return true;
	}
	return false;
}

bool IsSpecialNameChar(char c)
{
	return false;
}

bool IsNameChar(char c)
{
	switch (c)
	{
	CASE_NUM_CHAR
	CASE_WORD_CHAR
		return true;
	}
	return IsSpecialNameChar(c);
}
	
bool IsErrorChar(char c)
{
	switch (c)
	{
	CASE_WS_CHAR
	CASE_NUM_CHAR
	CASE_WORD_CHAR
	case '\'':
	case '\"':
	case '@':
	case '$':
	case '(':
	case ')':
	case '{':
	case '}':
	case '[':
	case ']':
	case '.':
	case ',':
	case ':':
	case ';':
	case '?':
	case '+':
	case '-':
	case '*':
	case '/':
	case '%':
	case '=':
	case '<':
	case '>':
	case '!':
	case '&':
	case '|':
	case '~':
	case '^':
		return false;
	}
	return !IsSpecialNameChar(c);
}

class Bookmark
{
	friend class LexerBase;

	Bookmark(const char* ptr)
		: m_ptr(ptr)
	{
	}

	const char* m_ptr;
};

class LexerBase
{
protected:
	i32 Count() const
	{
		return m_last - m_current;
	}

	char Peek(i32 offset = 0) const
	{
		Assert(offset < Count());
		return m_current[offset];
	}

	void Eat(i32 count = 1)
	{
		Assert(count > 0 && count <= Count());
		m_current += count;
	}

	i32 GetLineIndex() const
	{
		return m_lineIndex;
	}

	void BreakLine()
	{
		++m_lineIndex;
		m_lineFirst = m_current;
	}

	SourcePos GetSourcePos() const
	{
		return SourcePos(m_lineIndex, m_current - m_lineFirst);
	}

	Bookmark CreateBookmark()
	{
		return Bookmark(m_current);
	}

	i32 Distance(Bookmark first) const
	{
		Assert(first.m_ptr >= m_first && first.m_ptr <= m_current);
		return m_current - first.m_ptr;
	}

	i32 Distance(Bookmark first, Bookmark last) const
	{
		Assert(first.m_ptr >= m_first && first.m_ptr <= last.m_ptr && last.m_ptr <= m_current);
		return last.m_ptr - first.m_ptr;
	}

	std::string_view ExtractString(Bookmark first)
	{
		Assert(first.m_ptr >= m_first && first.m_ptr <= m_current);
		return std::string_view(first.m_ptr, m_current - first.m_ptr);
	}

	std::string_view ExtractString(Bookmark first, Bookmark last)
	{
		Assert(first.m_ptr >= m_first && first.m_ptr <= last.m_ptr && last.m_ptr <= m_current);
		return std::string_view(first.m_ptr, last.m_ptr - first.m_ptr);
	}

	std::string_view CreateString(Bookmark first)
	{
		return CreateString(ExtractString(first));
	}

	std::string_view CreateString(Bookmark first, Bookmark last)
	{
		return CreateString(ExtractString(first, last));
	}

	std::string_view CreateString(std::string_view string)
	{
		return m_treeContext->CreateString(string);
	}

	std::string_view CreateString(const StringBuilder& stringBuilder)
	{
		return m_treeContext->CreateString(stringBuilder);
	}

	template<typename T, typename... TArgs>
	T* CreateLexeme(SourcePos lexeme, TArgs&&... args)
	{
		static_assert(std::is_base_of_v<Lexeme, T>);
		return m_treeContext->CreateSyntax<T>(lexeme, std::forward<TArgs>(args)...);
	}

	template<typename T, typename... TArgs>
	T* CreateToken(TokenInfo tokenInfo, TArgs&&... args)
	{
		static_assert(std::is_base_of_v<Token, T>);
		return m_treeContext->CreateSyntax<T>(tokenInfo, std::forward<TArgs>(args)...);
	}

	template<typename T, typename TIterator>
	Span<T*> CreateLexemeList(TIterator first, TIterator last)
	{
		static_assert(std::is_base_of_v<Lexeme, T>);
		return m_treeContext->CreateSyntaxList<T>(first, last);
	}

	template<typename T, typename TRange>
	Span<T*> CreateLexemeList(const TRange& range)
	{
		using std::begin; using std::end;
		return CreateLexemeList<T>(begin(range), end(range));
	}

protected:
	LexerBase(const char* first, const char* last, SyntaxTreeContext* treeContext)
		: m_first(first), m_last(last), m_current(first), m_treeContext(treeContext), m_lineFirst(first)
	{
	}

private:
	const char* m_first;
	const char* m_last;
	const char* m_current;

	i32 m_lineIndex = 0;
	const char* m_lineFirst;

	SyntaxTreeContext* m_treeContext;
};

class Lexer : LexerBase
{
public:
	Lexer(std::string_view text, SyntaxTreeContext* treeContext)
		: LexerBase(text.data(), text.data() + text.size(), treeContext)
	{
	}

	Token* ScanToken()
	{
		ScanTrivia(m_trivia, true);

		SourcePos lexeme = GetSourcePos();

		if (Count() == 0)
			return CreateToken<EofToken>(lexeme);
		
		auto next = Peek();
		switch (next)
		{
		case '\'':
			return ScanCharLiteral();

		case '\"':
			return ScanStringLiteral();

		case '@':
			if (Count() > 1 && Peek(1) == '"')
				return ScanVerbatimStringLiteral();
			return ScanWord(true);

		case '$':
			Eat();
			return CreateSymbolToken(lexeme, Symbol::Dollar);

		case '(':
			Eat();
			return CreateSymbolToken(lexeme, Symbol::LParen);

		case ')':
			Eat();
			return CreateSymbolToken(lexeme, Symbol::RParen);

		case '{':
			Eat();
			return CreateSymbolToken(lexeme, Symbol::LBrace);

		case '}':
			Eat();
			return CreateSymbolToken(lexeme, Symbol::RBrace);

		case '[':
			Eat();
			return CreateSymbolToken(lexeme, Symbol::LBrack);

		case ']':
			Eat();
			return CreateSymbolToken(lexeme, Symbol::RBrack);

		case '.':
			if (Count() > 1)
			{
				switch (Peek(1))
				{
				case '.':
					if (Count() > 2 && Peek(2) == '.')
					{
						Eat(3);
						return CreateSymbolToken(lexeme, Symbol::TripleEllipsis);
					}

					Eat(2);
					return CreateSymbolToken(lexeme, Symbol::DoubleEllipsis);

				CASE_NUM_CHAR
					return ScanNumericLiteral();
				}
			}

			Eat();
			return CreateSymbolToken(lexeme, Symbol::Dot);

		case ',':
			Eat();
			return CreateSymbolToken(lexeme, Symbol::Comma);

		case ':':
			Eat();
			if (Count() > 0 && Peek() == ':')
			{
				Eat();
				return CreateSymbolToken(lexeme, Symbol::Scope);
			}
			return CreateSymbolToken(lexeme, Symbol::Colon);

		case ';':
			Eat();
			return CreateSymbolToken(lexeme, Symbol::Semicolon);

		case '?':
			Eat();
			if (Count() > 0 && Peek() == '?')
			{
				Eat();
				return CreateSymbolToken(lexeme, Symbol::Coalescing);
			}
			return CreateSymbolToken(lexeme, Symbol::Question);

		case '+':
			Eat();
			if (Count() > 0)
			{
				switch (Peek())
				{
				case '=':
					Eat();
					return CreateSymbolToken(lexeme, Symbol::AssignAdd);

				case '+':
					Eat();
					return CreateSymbolToken(lexeme, Symbol::Increment);
				}
			}
			return CreateSymbolToken(lexeme, Symbol::Add);

		case '-':
			Eat();
			if (Count() > 0)
			{
				switch (Peek())
				{
				case '=':
					Eat();
					return CreateSymbolToken(lexeme, Symbol::AssignSub);

				case '+':
					Eat();
					return CreateSymbolToken(lexeme, Symbol::Decrement);

				case '>':
					Eat();
					return CreateSymbolToken(lexeme, Symbol::Arrow);
				}
			}
			return CreateSymbolToken(lexeme, Symbol::Sub);

		case '*':
			Eat();
			if (Count() > 0 && Peek() == '=')
			{
				Eat();
				return CreateSymbolToken(lexeme, Symbol::AssignMul);
			}
			return CreateSymbolToken(lexeme, Symbol::Mul);

		case '/':
			Eat();
			if (Count() > 0 && Peek() == '=')
			{
				Eat();
				return CreateSymbolToken(lexeme, Symbol::AssignDiv);
			}
			return CreateSymbolToken(lexeme, Symbol::Div);

		case '%':
			Eat();
			if (Count() > 0 && Peek() == '=')
			{
				Eat();
				return CreateSymbolToken(lexeme, Symbol::AssignMod);
			}
			return CreateSymbolToken(lexeme, Symbol::Mod);

		case '=':
			Eat();
			if (Count() > 0)
			{
				switch (Peek())
				{
				case '=':
					Eat();
					return CreateSymbolToken(lexeme, Symbol::Equal);

				case '>':
					Eat();
					return CreateSymbolToken(lexeme, Symbol::Lambda);
				}
			}
			return CreateSymbolToken(lexeme, Symbol::Assign);

		case '<':
			Eat();
			if (Count() > 0)
			{
				switch (Peek())
				{
				case '=':
					Eat();
					return CreateSymbolToken(lexeme, Symbol::LessOrEqual);

				case '<':
					Eat();
					if (Count() > 1 && Peek(1) == '=')
					{
						Eat();
						return CreateSymbolToken(lexeme, Symbol::AssignLhs);
					}
					return CreateSymbolToken(lexeme, Symbol::LeftShift);
				}
			}
			return CreateSymbolToken(lexeme, Symbol::Less);

		case '>':
			Eat();
			return CreateSymbolToken(lexeme, Symbol::Greater);

		case '!':
			Eat();
			if (Count() > 0 && Peek() == '=')
			{
				Eat();
				return CreateSymbolToken(lexeme, Symbol::NotEqual);
			}
			return CreateSymbolToken(lexeme, Symbol::LogicalNot);

		case '&':
			Eat();
			if (Count() > 0)
			{
				switch (Peek())
				{
				case '=':
					Eat();
					return CreateSymbolToken(lexeme, Symbol::AssignAnd);

				case '&':
					Eat();
					return CreateSymbolToken(lexeme, Symbol::LogicalAnd);
				}
			}
			return CreateSymbolToken(lexeme, Symbol::And);

		case '|':
			Eat();
			if (Count() > 0)
			{
				switch (Peek())
				{
				case '=':
					Eat();
					return CreateSymbolToken(lexeme, Symbol::AssignOr);

				case '|':
					Eat();
					return CreateSymbolToken(lexeme, Symbol::LogicalOr);
				}
			}
			return CreateSymbolToken(lexeme, Symbol::Or);

		case '~':
			Eat();
			return CreateSymbolToken(lexeme, Symbol::Not);

		case '^':
			Eat();
			if (Count() > 0 && Peek() == '=')
			{
				Eat();
				return CreateSymbolToken(lexeme, Symbol::AssignXor);
			}
			return CreateSymbolToken(lexeme, Symbol::Xor);

		CASE_NUM_CHAR
			return ScanNumericLiteral();

		default:
			Assert(IsSpecialNameChar(next));
		
		CASE_WORD_CHAR
			return ScanWord(false);
		}
	}

private:
	BlockCommentTrivia* ScanBlockCommentTrivia()
	{
		SourcePos lexeme = GetSourcePos();

		Assert(Count() >= 2 && Peek(0) == '/' && Peek(1) == '*');
		Eat(2);

		Bookmark start = CreateBookmark();
		Bookmark end = start;

		while (Count() > 0)
		{
			switch (Peek())
			{
			case '\n':
				Eat();
				BreakLine();
				break;

			case '*':
				if (Count() > 1 && Peek(1) == '/')
				{
					Eat(2);
					goto exit;
				}
				Eat();
				break;

			default:
				Eat();
			}
		}

		//TODO: error: Fatal::OpenComment

	exit:
		auto string = CreateString(start, end);
		return CreateLexeme<BlockCommentTrivia>(lexeme, string);
	}

	LineCommentTrivia* ScanLineCommentTrivia()
	{
		SourcePos lexeme = GetSourcePos();

		Assert(Count() > 1 && Peek(0) == '/' && Peek(1) == '/');
		Eat(2);

		Bookmark start = CreateBookmark();
		Bookmark end = start;

		std::string_view newline;

		while (Count() > 0)
		{
			switch (Peek())
			{
			case '\n':
				end = CreateBookmark();
				newline = "\n";
				Eat();
				BreakLine();
				break;

			case '\r':
				if (Count() > 1 && Peek(1) == '\n')
				{
					end = CreateBookmark();
					newline = "\r\n";
					Eat(2);
					BreakLine();
					break;
				}

			default:
				Eat();
				continue;
			}
			break;
		}

		auto string = CreateString(start, end);
		return CreateLexeme<LineCommentTrivia>(lexeme, string, newline);
	}

	WhiteSpaceTrivia* ScanWhiteSpaceTrivia()
	{
		SourcePos lexeme = GetSourcePos();

		Assert(Count() > 0 && IsWhiteSpaceChar(Peek()));

		Bookmark start = CreateBookmark();
		Bookmark end = start;

		std::string_view newline;

		do
		{
			switch (Peek())
			{
			case '\n':
				end = CreateBookmark();
				newline = "\n";
				Eat();
				BreakLine();
				break;

			case '\r':
				if (Count() > 1 && Peek(1) == '\n')
				{
					end = CreateBookmark();
					newline = "\r\n";
					Eat(2);
					BreakLine();
					break;
				}

			case ' ':
			case '\t':
			case '\v':
			case '\f':
				Eat();
				continue;
			}
			break;
		} while (Count() > 0);

		auto string = CreateString(start, end);
		return CreateLexeme<WhiteSpaceTrivia>(lexeme, string, newline);
	}

	ErrorCharTrivia* ScanErrorCharTrivia()
	{
		SourcePos lexeme = GetSourcePos();

		Assert(Count() > 0 && IsErrorChar(Peek()));

		Bookmark start = CreateBookmark();

		do Eat();
		while (IsErrorChar(Peek()));

		auto string = CreateString(start);
		return CreateLexeme<ErrorCharTrivia>(lexeme, string);
	}

	void ScanTrivia(std::vector<Trivia*>& vector, bool multiLine)
	{
		i32 lineIndex = GetLineIndex();

		while (Count() > 0)
		{
			auto next = Peek();
			switch (next)
			{
			case ' ':
			case '\t':
			case '\v':
			case '\f':
			case '\r':
			case '\n':
				vector.push_back(ScanWhiteSpaceTrivia());
				if (!multiLine && GetLineIndex() != lineIndex)
					return;
				break;

			case '/':
				if (Count() < 2)
					return;

				switch (Peek(1))
				{
				case '/':
					vector.push_back(ScanLineCommentTrivia());
					if (!multiLine) return;
					break;

				case '*':
					vector.push_back(ScanBlockCommentTrivia());
					if (!multiLine && GetLineIndex() != lineIndex)
						return;
					break;

				default:
					return;
				}
				break;

			default:
				if (!IsErrorChar(next))
					return;

				vector.push_back(ScanErrorCharTrivia());
			}
		}
	}


	WordToken* ScanWord(bool verbatim, bool subToken = false)
	{
		SourcePos lexeme = GetSourcePos();

		Assert(Count() > 0);

		if (verbatim)
		{
			Assert(Peek() == '@');
			Eat();
		}

		Bookmark start = CreateBookmark();
		bool name = false;

		do
		{
			auto next = Peek();
			switch (next)
			{
			default:
				if (!IsSpecialNameChar(next))
					goto exit;
				name = true;

			CASE_NUM_CHAR
				CASE_WORD_CHAR
				Eat();
				break;
			}
		} while (Count() > 0);

	exit:
		auto string = ExtractString(start);
		Assert(!string.empty() || verbatim);

		Keyword keyword;
		if (!verbatim && !name && ResolveKeyword(string, keyword))
		{
			if (subToken) return CreateSubToken<KeywordToken>(lexeme, keyword);
			return CreateToken<KeywordToken>(lexeme, keyword);
		}

		string = CreateString(string);
		if (subToken) return CreateSubToken<NameToken>(lexeme, string, verbatim);
		return CreateToken<NameToken>(lexeme, string, verbatim);
	}

	WordToken* ScanLiteralSuffix()
	{
		if (Count() > 0)
		{
			auto next = Peek();
			if (next == '@')
				return ScanWord(true, true);
			else if (IsNameChar(next))
				return ScanWord(false, true);
		}
		return nullptr;
	}

	std::string_view ScanEscapeSequence()
	{
		Assert(Count() > 0);

		switch (Peek())
		{
		case '\'':
			Eat();
			return "\'";

		case '\"':
			Eat();
			return "\"";

		case '\\':
			Eat();
			return "\\";

		case '0':
			Eat();
			return "\0";

		case 'a':
			Eat();
			return "\a";

		case 'b':
			Eat();
			return "\b";

		case 'f':
			Eat();
			return "\f";

		case 'n':
			Eat();
			return "\n";

		case 'r':
			Eat();
			return "\r";

		case 't':
			Eat();
			return "\t";

		case 'v':
			Eat();
			return "\v";

		case 'u':
			Eat();
			//TODO: \uxxxx
			return "";

		case 'U':
			Eat();
			//TODO: \Uxxxxxxxx
			return "";
		}

		//TODO: Set error: Error::InvalidEscapeSequence
		return "";
	}

	CharLiteralToken* ScanCharLiteral()
	{
		SourcePos lexeme = GetSourcePos();

		Assert(Count() > 0 && Peek() == '\'');
		Eat();

		Bookmark start = CreateBookmark();
		Bookmark end = start;

		StringBuilder builder;
		while (Count() > 0)
		{
			switch (Peek())
			{
			case '\'':
				end = CreateBookmark();
				Eat();
				goto exit;

			case '\\':
				builder += ExtractString(start);
				Eat();

				if (Count() > 0)
					builder += ScanEscapeSequence();
				else; //TODO: error: Error::InvalidEscapeSequence

				start = CreateBookmark();
				break;

			default:
				Eat();
			}
		}

		//TODO: error: Fatal::OpenCharacterLiteral
		end = CreateBookmark();

	exit:
		builder += ExtractString(start, end);
		std::string_view string = CreateString(builder);

		WordToken* type = ScanLiteralSuffix();
		return CreateToken<CharLiteralToken>(lexeme, string, type);
	}

	StringLiteralToken* ScanStringLiteral()
	{
		SourcePos lexeme = GetSourcePos();

		Assert(Count() > 0 && Peek() == '\"');
		Eat();

		Bookmark start = CreateBookmark();
		Bookmark end = start;

		StringBuilder builder;
		while (Count() > 0)
		{
			switch (Peek())
			{
			case '\"':
				end = CreateBookmark();
				Eat();
				goto exit;

			case '\\':
				builder += ExtractString(start);
				Eat();

				if (Count() > 0)
					builder += ScanEscapeSequence();
				else; //TODO: error: Error::InvalidEscapeSequence

				start = CreateBookmark();
				break;

			case '\r':
				Eat();
				if (Count() == 0 || Peek() != '\n')
					break;

			case '\n':
				//TODO: error: Error::NewlineInStringLiteral
				Eat();
				BreakLine();
				break;

			default:
				Eat();
			}
		}

		//TODO: error: Fatal::OpenStringLiteral
		end = CreateBookmark();

	exit:
		builder += ExtractString(start, end);
		std::string_view string = CreateString(builder);

		WordToken* type = ScanLiteralSuffix();
		return CreateToken<StringLiteralToken>(lexeme, string, type);
	}

	StringLiteralToken* ScanVerbatimStringLiteral()
	{
		SourcePos lexeme = GetSourcePos();

		Assert(Count() > 1 && Peek(0) == '@' && Peek(1) == '\"');
		Eat(2);

		Bookmark start = CreateBookmark();
		Bookmark end = start;

		StringBuilder builder;
		while (Count() > 0)
		{
			switch (Peek())
			{
			case '\"':
				if (Count() > 1 && Peek(1) == '\"')
				{
					builder += ExtractString(start);
					builder += "\"";
					Eat(2);
					start = CreateBookmark();
					break;
				}

				end = CreateBookmark();
				Eat();
				goto exit;

			case '\r':
				if (Count() > 1 && Peek(1) == '\n')
				{
					builder += ExtractString(start);
					builder += "\n";
					Eat(2);
					BreakLine();
					start = CreateBookmark();
					break;
				}

			case '\n':
				Eat();
				BreakLine();
				break;

			default:
				Eat();
			}
		}

		//TODO: error: Fatal::OpenStringLiteral
		end = CreateBookmark();

	exit:
		builder += ExtractString(start, end);
		auto string = CreateString(builder);

		WordToken* type = ScanLiteralSuffix();
		return CreateToken<StringLiteralToken>(lexeme, string, type);
	}


	bool ScanInteger(i32 radix, i64* accumulate)
	{
		Assert(radix == 2 || radix == 8 || radix == 10 || radix == 16);
		Bookmark start = CreateBookmark();

		auto acc = *accumulate;
		while (Count() > 0)
		{
			i32 digit = -1;
			auto next = Peek();
			if (radix > 10)
			{
				if (next >= '0' && next < '0' + 10)
					digit = next - '0';
				else if (next >= 'a' && next < 'a' + (decltype(next))radix - 10)
					digit = 10 + (next - 'a');
				else if (next >= 'A' && next < 'A' + (decltype(next))radix - 10)
					digit = 10 + (next - 'A');
			}
			else
			{
				if (next >= '0' && next < '0' + (decltype(next))radix)
					digit = next - '0';
			}

			if (digit < 0)
				break;

			Eat();
			acc = acc * radix + digit;
		}

		if (Distance(start) == 0)
			return false;

		*accumulate = acc;
		return true;
	}

	NumericLiteralToken* ScanNumericLiteral()
	{
		SourcePos lexeme = GetSourcePos();

		Assert(Count() > 0);
		Rational result;

		i32 radix = 10;
		if (Peek() == '0')
		{
			Eat();

			if (Count() > 0)
			{
				switch (Peek())
				{
				case 'b':
				case 'B':
					Eat();
					radix = 2;
					break;

				case 'o':
				case 'O':
					Eat();
					radix = 8;
					break;

				case 'x':
				case 'X':
					Eat();
					radix = 16;
					break;
				}
			}
		}

		Bookmark start = CreateBookmark();
		while (Count() > 0 && Peek() == '0')
			Eat();

		i64 num = 0;
		ScanInteger(radix, &num);

		i32 fracDigits = 0;
		if (Count() > 0 && Peek() == '.')
		{
			Eat();
			while (Count() > 0 && Peek() == '0')
				Eat();

			start = CreateBookmark();
			if (ScanInteger(radix, &num))
				fracDigits = Distance(start);
		}
		result.SetNumerator(num);

		i32 base = -1;
		if (Count() > 0)
		{
			switch (Peek())
			{
			case 'e':
			case 'E':
				Eat();
				base = 10;
				break;

			case 'p':
			case 'P':
				Eat();
				base = 2;
				break;
			}

			if (base > 0)
			{
				bool pos = true;
				if (Count() > 0)
				{
					switch (Peek())
					{
					case '-':
						pos = false;
					case '+':
						Eat();
						break;
					}
				}

				i64 exp = 0;
				if (Count() > 0 && ScanInteger(10, &exp))
				{
					//TODO: pow
					i64 scale = 1;
					for (i64 i = 0; i < exp; ++i)
						scale *= base;
					if (pos) result *= scale;
					else result /= scale;
				}
				else; //TODO: error: Error::InvalidNumericExponent
			}
		}

		if (fracDigits > 0)
		{
			//TODO: pow
			i64 scale = 1;
			for (i64 i = 0; i < fracDigits; ++i)
				scale *= radix;
			result /= scale;
		}

		WordToken* type = ScanLiteralSuffix();
		return CreateToken<NumericLiteralToken>(lexeme, result, type);
	}

	template<typename T, typename... TArgs>
	T* CreateToken(SourcePos lexeme, TArgs&&... args)
	{
		std::vector<Trivia*> trivia;
		ScanTrivia(trivia, false);

		Span<Trivia*> leadingTrivia = CreateLexemeList<Trivia>(m_trivia);
		Span<Trivia*> trailingTrivia = CreateLexemeList<Trivia>(trivia);

		m_trivia.clear();

		TokenInfo tokenInfo{ lexeme, leadingTrivia, trailingTrivia };
		return LexerBase::CreateToken<T>(tokenInfo, std::forward<TArgs>(args)...);
	}

	template<typename T, typename... TArgs>
	T* CreateSubToken(SourcePos lexeme, TArgs&&... args)
	{
		TokenInfo tokenInfo{ lexeme, Span<Trivia*>(), Span<Trivia*>() };
		return LexerBase::CreateToken<T>(tokenInfo, std::forward<TArgs>(args)...);
	}

	SymbolToken* CreateSymbolToken(SourcePos lexeme, Symbol symbol)
	{
		return CreateToken<SymbolToken>(lexeme, symbol);
	}

	std::vector<Trivia*> m_trivia;
};

} // namespace

struct CoroGLL::Private::TokenListAttorney
{
	static TokenList CreateTokenList(std::vector<Ast::Token*>&& tokens, SyntaxTreeContext&& treeContext)
	{
		return TokenList(std::move(tokens), std::move(treeContext));
	}
};

std::vector<CoroGLL::Ast::Token*> CoroGLL::Private::Lex(std::string_view text, SyntaxTreeContext* treeContext)
{
	std::vector<Ast::Token*> tokens;
	Lexer lexer(text, treeContext);

	while (true)
	{
		Ast::Token* token = lexer.ScanToken();
		tokens.push_back(token);

		if (token->Kind() == SyntaxKind::EofToken)
			break;
	}

	return tokens;
}

CoroGLL::TokenList CoroGLL::Lex(std::string_view text)
{
	SyntaxTreeContext treeContext;
	std::vector<Ast::Token*> tokens = Lex(text, &treeContext);
	return Private::TokenListAttorney::CreateTokenList(
		std::move(tokens), std::move(treeContext));
}
