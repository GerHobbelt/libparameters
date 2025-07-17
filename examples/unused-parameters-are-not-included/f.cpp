
// var f

#include <parameters/parameters.h>

#include "var-a-b-c-d.hpp"

using namespace parameters;

// declare a parameter instance (variable) and hook it up to the global set:
UserDefValueParam f({.x = 3, .y = 7}, "f", "example variable `f`");
