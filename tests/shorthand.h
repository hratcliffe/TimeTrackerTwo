#include <catch2/matchers/catch_matchers_floating_point.hpp>

#ifndef __shorthand
#define __shorthand
static const float margin = 0.001; // Float margin
static auto inline WithinAbs = Catch::Matchers::WithinAbs;
#endif