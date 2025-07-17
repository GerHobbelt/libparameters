
// var f

#include <parameters/parameters.h>

#include "var-a-b-c-d.hpp"

using namespace parameters;

// declare a parameter instance (variable) and hook it up to the global set:
#if 0
UserDefRefParam f1({.x = 3, .y = 7}, "g", "example variable `g`");  //--> compiler error (as we WANT here as this would otherwise deref a *temporary UserDefArg class instance* and cause a lot of trouble due to invalid memory accesses later on...
#endif

static UserDefArg f_val{.x = 5, .y = 11};

UserDefRefParam g(f_val, "g", "example variable `g`");
