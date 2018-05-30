#pragma once

#include <cassert>

#define DEBUG 1

#if DEBUG
#	define Assert(...) assert(__VA_ARGS__)
#else
#	define Assert(...) ((void)0)
#endif
