#pragma once

#include "../Core/Types.hpp"

#include <string_view>

#define COROGLL_KEYWORD(X)            \
	X( Abstract  , "abstract"  )    \
	X( Alignof   , "alignof"   )    \
	X( And       , "and"       )    \
	X( As        , "as"        )    \
	X( Async     , "async"     )    \
	X( Await     , "await"     )    \
	X( Bitcast   , "bitcast"   )    \
	X( Break     , "break"     )    \
	X( Bool      , "bool"      )    \
	X( Case      , "case"      )    \
	X( Cast      , "cast"      )    \
	X( Concept   , "concept"   )    \
	X( Const     , "const"     )    \
	X( Continue  , "continue"  )    \
	X( Contract  , "contract"  )    \
	X( Declof    , "declof"    )    \
	X( Default   , "default"   )    \
	X( Do        , "do"        )    \
	X( Dyncast   , "dyncast"   )    \
	X( Else      , "else"      )    \
	X( F32       , "f32"       )    \
	X( F64       , "f64"       )    \
	X( False     , "false"     )    \
	X( Final     , "final"     )    \
	X( For       , "for"       )    \
	X( Goto      , "goto"      )    \
	X( I8        , "i8"        )    \
	X( I16       , "i16"       )    \
	X( I32       , "i32"       )    \
	X( I64       , "i64"       )    \
	X( Iptr      , "iword"      )    \
	X( If        , "if"        )    \
	X( Import    , "import"    )    \
	X( In        , "in"        )    \
	X( Internal  , "internal"  )    \
	X( Nameof    , "nameof"    )    \
	X( Null      , "null"      )    \
	X( Operator  , "operator"  )    \
	X( Or        , "or"        )    \
	X( Override  , "override"  )    \
	X( Private   , "private"   )    \
	X( Protected , "protected" )    \
	X( Public    , "public"    )    \
	X( Return    , "return"    )    \
	X( Sizeof    , "sizeof"    )    \
	X( Static    , "static"    )    \
	X( Struct    , "struct"    )    \
	X( Switch    , "switch"    )    \
	X( Template  , "template"  )    \
	X( This      , "this"      )    \
	X( True      , "true"      )    \
	X( Typeof    , "typeof"    )    \
	X( U8        , "u8"        )    \
	X( U16       , "u16"       )    \
	X( U32       , "u32"       )    \
	X( U64       , "u64"       )    \
	X( Uptr      , "uword"      )    \
	X( Using     , "using"     )    \
	X( Virtual   , "virtual"   )    \
	X( Void      , "void"      )    \
	X( While     , "while"     )    \
	X( Yield     , "yield"     )    \
