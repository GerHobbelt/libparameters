
// var b

#include <parameters/parameters.h>

#include "var-a-b-c-d.hpp"

using namespace parameters;

// declare a parameter instance (variable) and hook it up to the global set:
BoolParam b(false, "b", "example variable `b`", GlobalParams());
