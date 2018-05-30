#pragma once

#include "../Core/Types.hpp"

namespace CoroGLL::Ast
{
	struct SourcePos
	{
		SourcePos(i32 line, i32 column)
			: line(line), column(column)
		{
		}

		i32 Line() const
		{
			return line;
		}

		i32 Column() const
		{
			return column;
		}

	private:
		i32 line;
		i32 column;
	};
}
