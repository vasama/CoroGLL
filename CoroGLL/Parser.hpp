#pragma once

#include "SyntaxTree.hpp"

#include <string_view>

namespace CoroGLL {

SyntaxTree ParseExpression(std::string_view text);

} // namespace CoroGLL
