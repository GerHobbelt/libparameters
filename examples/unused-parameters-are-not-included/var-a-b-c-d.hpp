
#pragma once

#include <parameters/parameters.h>

extern ::parameters::BoolParam a;
extern ::parameters::BoolParam b;
extern ::parameters::IntParam c;
extern ::parameters::DoubleParam d;
extern ::parameters::StringParam e;

struct UserDefArg {
	int x;
	int y;
};

using UserDefValueParam = ::parameters::ValueTypedParam<UserDefArg, ::parameters::BasicVectorParamParseAssistant>;

extern UserDefValueParam f;

using UserDefRefParam = ::parameters::RefTypedParam<UserDefArg, ::parameters::BasicVectorParamParseAssistant>;

extern UserDefRefParam g;

