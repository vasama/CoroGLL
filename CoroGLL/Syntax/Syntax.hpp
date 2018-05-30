#pragma once

#include "SyntaxKind.hpp"
#include "../Core/Span.hpp"

#if 0
#ifndef COROGLL_SYNTAX_DEBUGPRINT_ENABLE
#	define COROGLL_SYNTAX_DEBUGPRINT_ENABLE 1
#endif

#if COROGLL_SYNTAX_DEBUGPRINT_ENABLE
#	include <ostream>
#	define COROGLL_SYNTAX_DEBUGPRINT() void DebugPrint(Syntax::DebugPrintInfo info) const override
#else
#	define COROGLL_SYNTAX_DEBUGPRINT() void DebugPrint(Syntax::DebugPrintInfo info) const
#endif
#else
#define COROGLL_SYNTAX_DEBUGPRINT()
#endif

namespace CoroGLL::Ast
{
	struct Syntax
	{
		typedef Syntax CommonType;

		SyntaxKind Kind() const
		{
			return syntaxKind;
		}

#if COROGLL_SYNTAX_DEBUGPRINT_ENABLE && 0
		void DebugPrint(std::ostream& stream) const;
#endif
		
	protected:
		Syntax(SyntaxKind syntaxKind)
			: syntaxKind(syntaxKind)
		{
		}

		struct DebugPrintInfo
		{
			std::ostream& stream;
			i32 indent = 0;

			void PrintIndent() const;
		};

#if 0
#if COROGLL_SYNTAX_DEBUGPRINT_ENABLE
		virtual
#endif
		void DebugPrint(DebugPrintInfo info) const;
		void DebugPrint(DebugPrintInfo info, const Syntax* syntax) const;
		void DebugPrint(DebugPrintInfo info, Span<const Syntax*> syntax) const;
#endif

	private:
		SyntaxKind syntaxKind;
	};
}
