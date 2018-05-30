#pragma once

#include "SyntaxTree.hpp"
#include "Syntax/Token.hpp"

#include <string_view>
#include <utility>
#include <vector>

namespace CoroGLL::Private {

struct TokenListAttorney;

std::vector<Ast::Token*> Lex(std::string_view text, SyntaxTreeContext* treeContext);

} // namespace CoroGLL::Private

namespace CoroGLL {

class TokenList
{
public:
	i32 Count() const
	{
		return m_tokens.size();
	}

	Ast::Token* operator[](i32 index) const
	{
		return m_tokens[index];
	}

	Span<Ast::Token* const> AsSpan() const
	{
		return Span<Ast::Token* const>(m_tokens.data(), m_tokens.size());
	}
	
private:
	TokenList(std::vector<Ast::Token*> tokens, Private::SyntaxTreeContext treeContext)
		: m_tokens(std::move(tokens)), m_treeContext(std::move(treeContext))
	{
	}

	std::vector<Ast::Token*> m_tokens;
	Private::SyntaxTreeContext m_treeContext;

	friend struct Private::TokenListAttorney;

	friend Ast::Token* const* begin(const TokenList& list)
	{
		return list.m_tokens.data();
	}

	friend Ast::Token* const* end(const TokenList& list)
	{
		return list.m_tokens.data() + list.m_tokens.size();
	}
};

TokenList Lex(std::string_view text);

} // namespace CoroGLL
