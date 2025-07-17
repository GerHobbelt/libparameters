
// var d

#include <parameters/parameters.h>

#include "var-a-b-c-d.hpp"

using namespace parameters;

// declare a parameter instance (variable) and hook it up to the global set:
DoubleParam d(3.1415, "d", "example variable `d`");
