
// var c

#include <parameters/parameters.h>

#include "var-a-b-c-d.hpp"

using namespace parameters;

// declare a parameter instance (variable) and hook it up to the global set:
IntParam c(42, "c", "example variable `c`", GlobalParams());
