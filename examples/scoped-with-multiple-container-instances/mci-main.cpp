
// showcases the scenario where all parameters are members of an application class which is instanced multiple times.
//
// the registered set identifies the parent of each parameter so each can be configured individually and independently. 

#include <parameters/parameters.h>

#include <iostream>
#include <format>
#include <cassert>

using namespace parameters;

static ParamsVector &ParamsManager(void) {
	static ParamsVector global_params("activity-callbacks"); // static auto-inits at startup
	return global_params;
}


#if defined(BUILD_MONOLITHIC)
#define main param_scoped_with_multiple_container_instances_example_main
#endif

extern "C"
int main(void) {
	BoolParam flag1(false, "f1", "bla bla bla bla", ParamsManager());
	IntParam flag2(999, "f2", "bla bla bla bla", ParamsManager());
	StringParam flag3("bugger", "f3", "bla bla bla bla", ParamsManager());

	flag1 = true;
	flag2.set_value("1234");
	flag3 = "default";

	std::cout << std::format("\n\nParameter values:\n\n  flag1 = {}\n  flag2 = {}\n  flag3 = \"{}\"\n\n", flag1.value(), flag2.value(), flag3.value());
	return 0;
}

