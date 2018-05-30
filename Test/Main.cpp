#include "Print.hpp"

#include "Parser.hpp"

#include <iostream>
#include <string>
#include <string_view>

using namespace CoroGLL;
using namespace CoroGLL::Ast;

int main(int argc, char** argv)
{
	std::string source = { std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>{} };

	SyntaxTree tree = ParseExpression(source);
	Print(std::cout, tree.GetRoot());
}
