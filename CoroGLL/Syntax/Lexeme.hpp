#pragma once

#include "SourcePos.hpp"

namespace CoroGLL::Ast
{
	struct Lexeme
	{
		SourcePos Pos() const
		{
			return pos;
		}

	protected:
		Lexeme(SourcePos pos)
			: pos(pos)
		{
		}

	private:
		SourcePos pos;
	};


}
