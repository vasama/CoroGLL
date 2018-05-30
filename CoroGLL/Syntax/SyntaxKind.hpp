#pragma once

#include "Keyword.hpp"
#include "../Core/Types.hpp"

#define COROGLL_TRIVIA(X) \
	X( BlockComment ) \
	X( ErrorChar    ) \
	X( LineComment  ) \
	X( WhiteSpace   ) \

#define COROGLL_TOKEN(X)  \
	X( CharLiteral    ) \
	X( Eof            ) \
	X( Name           ) \
	X( NumericLiteral ) \
	X( Missing        ) \
	X( StringLiteral  ) \

#define COROGLL_SYMBOL(X) \
	X( Add            ) \
	X( And            ) \
	X( Assign         ) \
	X( AssignAdd      ) \
	X( AssignAnd      ) \
	X( AssignDiv      ) \
	X( AssignLhs      ) \
	X( AssignMod      ) \
	X( AssignMul      ) \
	X( AssignNot      ) \
	X( AssignOr       ) \
	X( AssignSub      ) \
	X( AssignXor      ) \
	X( Arrow          ) \
	X( Colon          ) \
	X( Comma          ) \
	X( Decrement      ) \
	X( Div            ) \
	X( Dollar         ) \
	X( Dot            ) \
	X( DoubleEllipsis ) \
	X( TripleEllipsis ) \
	X( Equal          ) \
	X( Greater        ) \
	X( Increment      ) \
	X( LogicalOr      ) \
	X( LBrace         ) \
	X( LBrack         ) \
	X( LParen         ) \
	X( Lambda         ) \
	X( LeftShift      ) \
	X( Less           ) \
	X( LessOrEqual    ) \
	X( LogicalAnd     ) \
	X( LogicalNot     ) \
	X( Mod            ) \
	X( Mul            ) \
	X( Not            ) \
	X( NotEqual       ) \
	X( Coalescing     ) \
	X( Or             ) \
	X( Question       ) \
	X( RBrace         ) \
	X( RBrack         ) \
	X( RParen         ) \
	X( Scope          ) \
	X( Semicolon      ) \
	X( Sub            ) \
	X( Xor            ) \

#define COROGLL_EXPRESSION(X) \
	X( Cast          )      \
	X( Literal       )      \
	X( Meta          )      \
	X( Parenthesized )      \
	X( Ternary       )      \
	X( Wildcard      )      \
	X( Word          )      \

#define COROGLL_UNARY_OPERATOR(X) \
	X( Addressof        )       \
	X( Await            )       \
	X( Indirection      )       \
	X( LogicalNot       )       \
	X( Minus            )       \
	X( Not              )       \
	X( Nullable         )       \
	X( Plus             )       \
	X( Pointer          )       \
	X( PostfixDecrement )       \
	X( PostfixIncrement )       \
	X( PostfixEllipsis  )       \
	X( PrefixDecrement  )       \
	X( PrefixIncrement  )       \
	X( Reference        )       \

#define COROGLL_BINARY_OPERATOR(X)     \
	X( Addition                 )    \
	X( AdditionAssignment       )    \
	X( And                      )    \
	X( AndAssignment            )    \
	X( Assignment               )    \
	X( Division                 )    \
	X( DivisionAssignment       )    \
	X( Equal                    )    \
	X( GreaterThan              )    \
	X( GreaterThanOrEqual       )    \
	X( Modulo                   )    \
	X( ModuloAssignment         )    \
	X( Multiplication           )    \
	X( MultiplicationAssignment )    \
	X( NotAssignment            )    \
	X( NotEqual                 )    \
	X( Coalescing               )    \
	X( LeftShift                )    \
	X( LeftShiftAssignment      )    \
	X( LessThan                 )    \
	X( LessThanOrEqual          )    \
	X( LogicalAnd               )    \
	X( LogicalOr                )    \
	X( Or                       )    \
	X( OrAssignment             )    \
	X( RightShift               )    \
	X( RightShiftAssignment     )    \
	X( Subtraction              )    \
	X( SubtractionAssignment    )    \
	X( Xor                      )    \
	X( XorAssignment            )    \

#define COROGLL_INVOKE_OPERATOR(X) \
	X( Call           )          \
	X( Index          )          \
	X( Specialization )          \

#define COROGLL_ACCESS_OPERATOR(X) \
	X( Direct   )                \
	X( Indirect )                \
	X( Scope    )                \

#define COROGLL_SYNTAX(X)     \
	X( Argument           ) \
	X( ArgumentList       ) \
	X( ListExpression     ) \

namespace CoroGLL::Ast
{
	enum class SyntaxKind : u32
	{
		None = 0,

#define COROGLL_X(n) n ## Trivia,
		COROGLL_TRIVIA(COROGLL_X)
#undef COROGLL_X

#define COROGLL_X(n) n ## Token,
		COROGLL_TOKEN(COROGLL_X)
#undef COROGLL_X

#define COROGLL_X(n) n ## Symbol,
		COROGLL_SYMBOL(COROGLL_X)
#undef COROGLL_X

#define COROGLL_X(n, s) n ## Keyword,
		COROGLL_KEYWORD(COROGLL_X)
#undef COROGLL_X

#define COROGLL_X(n) n ## Expression,
		COROGLL_EXPRESSION(COROGLL_X)
		COROGLL_UNARY_OPERATOR(COROGLL_X)
		COROGLL_BINARY_OPERATOR(COROGLL_X)
		COROGLL_INVOKE_OPERATOR(COROGLL_X)
#undef COROGLL_X

#define COROGLL_X(n) n ## AccessExpression,
		COROGLL_ACCESS_OPERATOR(COROGLL_X)
#undef COROGLL_X

#define COROGLL_X(n) n ,
		COROGLL_SYNTAX(COROGLL_X)
#undef COROGLL_X

		LAngleSymbol = LessSymbol,
		RAngleSymbol = GreaterSymbol,
	};

	std::string_view ToString(SyntaxKind syntaxKind);
}

#define COROGLL_CASE_SYNTAXKIND_KEYWORD_X(n, s) case ::CoroGLL::Ast::SyntaxKind::n ## Keyword:
#define COROGLL_CASE_SYNTAXKIND_KEYWORD COROGLL_KEYWORD(COROGLL_CASE_SYNTAXKIND_KEYWORD_X)
